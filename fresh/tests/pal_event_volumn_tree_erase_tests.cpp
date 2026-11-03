#include "pal4/pal_event_volumn_manager.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IPalEventVolumnTreeNodeInsertRuntime {
    static constexpr std::uint32_t head = 0x1000u;
    static constexpr std::uint32_t nil = 0x9000u;
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::uint32_t> freed;
    std::uint32_t allocate_key = 0u;
    std::uint32_t SharedNil() override { return nil; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return memory.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        memory[address] = value;
    }
    void FreeNode(std::uint32_t node) override { freed.push_back(node); }
    std::uint32_t AllocateNode(std::uint32_t bytes) override {
        Check(bytes == 24u, "tree node allocation size");
        return allocate_key == 0u ? 0u : AddressForKey(allocate_key);
    }
    static std::uint32_t AddressForKey(std::uint32_t key) {
        return 0x2000u + key * 0x100u;
    }
    std::uint32_t Get(std::uint32_t node, std::uint32_t offset = 0u) {
        return memory.at(node + offset);
    }
    void Set(std::uint32_t node, std::uint32_t left,
             std::uint32_t parent, std::uint32_t right,
             std::uint32_t key, std::uint32_t color) {
        memory[node] = left;
        memory[node + 4u] = parent;
        memory[node + 8u] = right;
        memory[node + 12u] = key;
        memory[node + 16u] = key + 100u;
        memory[node + 20u] = color;
    }
    void Init() {
        Set(nil, nil, nil, nil, 0u, 1u);
        Set(head, head, head, head, 0u, 1u);
    }
};

struct InsertAdapter final : pal4::IPalEventVolumnTreeRuntime {
    Runtime& storage;
    std::uint32_t last_insert = 0u;
    explicit InsertAdapter(Runtime& source) : storage(source) {}
    std::uint32_t SharedNil() override { return storage.SharedNil(); }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return storage.ReadWord(address);
    }
    void InsertAt(pal4::PalEventVolumnState& state,
                  std::uint32_t child, std::uint32_t parent,
                  std::uint32_t key, std::uint32_t value) override {
        last_insert = pal4::PalEventVolumnManager::InsertTreeNode(
            state, child, parent, key, value, storage);
    }
};

std::uint32_t Address(std::uint32_t key) {
    return Runtime::AddressForKey(key);
}

int ValidateNode(Runtime& runtime, std::uint32_t node,
                 std::uint32_t parent, std::int32_t lower,
                 std::int32_t upper, std::set<std::uint32_t>& visited) {
    if (node == Runtime::nil) return 1;
    Check(visited.insert(node).second, "no cycles or repeated nodes");
    const auto key = static_cast<std::int32_t>(runtime.Get(node, 12u));
    Check(key > lower && key < upper, "signed BST ordering");
    Check(runtime.Get(node, 4u) == parent, "parent link");
    const auto left = runtime.Get(node);
    const auto right = runtime.Get(node, 8u);
    const auto color = runtime.Get(node, 20u);
    Check(color <= 1u, "node color is valid");
    if (color == 0u)
        Check(runtime.Get(left, 20u) == 1u &&
                  runtime.Get(right, 20u) == 1u,
              "red node has black children");
    const auto left_height = ValidateNode(runtime, left, node,
                                          lower, key, visited);
    const auto right_height = ValidateNode(runtime, right, node,
                                           key, upper, visited);
    Check(left_height == right_height, "equal black height");
    return left_height + (color == 1u ? 1 : 0);
}

void Validate(Runtime& runtime, const pal4::PalEventVolumnState& state,
              const std::set<std::uint32_t>& remaining) {
    const auto root = runtime.Get(Runtime::head, 4u);
    if (remaining.empty()) {
        Check(root == Runtime::nil &&
                  runtime.Get(Runtime::head) == Runtime::head &&
                  runtime.Get(Runtime::head, 8u) == Runtime::head,
              "empty tree head links");
    } else {
        Check(runtime.Get(root, 20u) == 1u, "root is black");
        Check(runtime.Get(root, 4u) == Runtime::head, "root parent is head");
        std::set<std::uint32_t> visited;
        (void)ValidateNode(runtime, root, Runtime::head,
                           -1000, 1000, visited);
        Check(visited.size() == remaining.size(), "all nodes reachable");
        auto minimum = root;
        while (runtime.Get(minimum) != Runtime::nil)
            minimum = runtime.Get(minimum);
        auto maximum = root;
        while (runtime.Get(maximum, 8u) != Runtime::nil)
            maximum = runtime.Get(maximum, 8u);
        Check(runtime.Get(Runtime::head) == minimum &&
                  runtime.Get(Runtime::head, 8u) == maximum,
              "head tracks extrema");
        for (const auto key : remaining)
            Check(visited.contains(Address(key)), "remaining key reachable");
    }
    Check(state.words[12u] == remaining.size(), "tree count");
    Check(runtime.Get(Runtime::nil, 20u) == 1u,
          "shared nil remains black");
}

void TestSevenNodeDeletion() {
    Runtime runtime;
    runtime.Init();
    runtime.Set(Runtime::head, Address(1u), Runtime::head,
                Address(7u), 0u, 1u);
    runtime.memory[Runtime::head + 4u] = Address(4u);
    runtime.Set(Address(4u), Address(2u), Runtime::head,
                Address(6u), 4u, 1u);
    runtime.Set(Address(2u), Address(1u), Address(4u),
                Address(3u), 2u, 1u);
    runtime.Set(Address(6u), Address(5u), Address(4u),
                Address(7u), 6u, 1u);
    for (const auto [key, parent] : {
             std::pair{1u, 2u}, {3u, 2u}, {5u, 6u}, {7u, 6u}})
        runtime.Set(Address(key), Runtime::nil, Address(parent),
                    Runtime::nil, key, 1u);
    pal4::PalEventVolumnState state;
    state.words[10u] = Runtime::head;
    state.words[12u] = 7u;
    std::set<std::uint32_t> remaining{1u, 2u, 3u, 4u, 5u, 6u, 7u};
    Validate(runtime, state, remaining);
    for (const auto key : {1u, 4u, 7u, 2u, 6u, 3u, 5u}) {
        const auto next = remaining.upper_bound(key);
        const auto expected = next == remaining.end()
                                  ? Runtime::head : Address(*next);
        Check(pal4::PalEventVolumnManager::EraseTreeNode(
                  state, Address(key), runtime) == expected &&
                  runtime.freed.back() == Address(key),
              "erase returns successor and frees requested node");
        remaining.erase(key);
        Validate(runtime, state, remaining);
    }
    Check(runtime.freed.size() == 7u, "all seven nodes freed once");
}

void TestRedSiblingRotation() {
    Runtime runtime;
    runtime.Init();
    runtime.memory[Runtime::head] = Address(1u);
    runtime.memory[Runtime::head + 4u] = Address(2u);
    runtime.memory[Runtime::head + 8u] = Address(5u);
    runtime.Set(Address(2u), Address(1u), Runtime::head,
                Address(4u), 2u, 1u);
    runtime.Set(Address(1u), Runtime::nil, Address(2u),
                Runtime::nil, 1u, 1u);
    runtime.Set(Address(4u), Address(3u), Address(2u),
                Address(5u), 4u, 0u);
    runtime.Set(Address(3u), Runtime::nil, Address(4u),
                Runtime::nil, 3u, 1u);
    runtime.Set(Address(5u), Runtime::nil, Address(4u),
                Runtime::nil, 5u, 1u);
    pal4::PalEventVolumnState state;
    state.words[10u] = Runtime::head;
    state.words[12u] = 5u;
    std::set<std::uint32_t> remaining{1u, 2u, 3u, 4u, 5u};
    Validate(runtime, state, remaining);
    Check(pal4::PalEventVolumnManager::EraseTreeNode(
              state, Address(1u), runtime) == Address(2u),
          "red sibling case returns next node");
    remaining.erase(1u);
    Validate(runtime, state, remaining);
}

void TestMirroredRedSiblingRotation() {
    Runtime runtime;
    runtime.Init();
    runtime.memory[Runtime::head] = Address(1u);
    runtime.memory[Runtime::head + 4u] = Address(4u);
    runtime.memory[Runtime::head + 8u] = Address(5u);
    runtime.Set(Address(4u), Address(2u), Runtime::head,
                Address(5u), 4u, 1u);
    runtime.Set(Address(2u), Address(1u), Address(4u),
                Address(3u), 2u, 0u);
    runtime.Set(Address(1u), Runtime::nil, Address(2u),
                Runtime::nil, 1u, 1u);
    runtime.Set(Address(3u), Runtime::nil, Address(2u),
                Runtime::nil, 3u, 1u);
    runtime.Set(Address(5u), Runtime::nil, Address(4u),
                Runtime::nil, 5u, 1u);
    pal4::PalEventVolumnState state;
    state.words[10u] = Runtime::head;
    state.words[12u] = 5u;
    std::set<std::uint32_t> remaining{1u, 2u, 3u, 4u, 5u};
    Validate(runtime, state, remaining);
    Check(pal4::PalEventVolumnManager::EraseTreeNode(
              state, Address(5u), runtime) == Runtime::head,
          "mirrored red sibling case returns end");
    remaining.erase(5u);
    Validate(runtime, state, remaining);
}

void TestNearRedChild() {
    Runtime runtime;
    runtime.Init();
    runtime.memory[Runtime::head] = Address(1u);
    runtime.memory[Runtime::head + 4u] = Address(2u);
    runtime.memory[Runtime::head + 8u] = Address(4u);
    runtime.Set(Address(2u), Address(1u), Runtime::head,
                Address(4u), 2u, 1u);
    runtime.Set(Address(1u), Runtime::nil, Address(2u),
                Runtime::nil, 1u, 1u);
    runtime.Set(Address(4u), Address(3u), Address(2u),
                Runtime::nil, 4u, 1u);
    runtime.Set(Address(3u), Runtime::nil, Address(4u),
                Runtime::nil, 3u, 0u);
    pal4::PalEventVolumnState state;
    state.words[10u] = Runtime::head;
    state.words[12u] = 4u;
    std::set<std::uint32_t> remaining{1u, 2u, 3u, 4u};
    Validate(runtime, state, remaining);
    Check(pal4::PalEventVolumnManager::EraseTreeNode(
              state, Address(1u), runtime) == Address(2u),
          "near red child case returns successor");
    remaining.erase(1u);
    Validate(runtime, state, remaining);
}

void TestInsertionOrders() {
    for (const auto& order : {
             std::vector<std::uint32_t>{1u, 2u, 3u, 4u, 5u, 6u, 7u},
             {7u, 6u, 5u, 4u, 3u, 2u, 1u},
             {4u, 2u, 6u, 1u, 3u, 5u, 7u},
             {3u, 1u, 2u}, {1u, 3u, 2u},
             {3u, 2u, 1u}, {1u, 2u, 3u}}) {
        Runtime runtime;
        runtime.Init();
        runtime.memory[Runtime::head + 4u] = Runtime::nil;
        pal4::PalEventVolumnState state;
        state.words[10u] = Runtime::head;
        std::set<std::uint32_t> inserted;
        for (const auto key : order) {
            auto child = runtime.Get(Runtime::head, 4u);
            auto parent = Runtime::head;
            while (child != Runtime::nil) {
                parent = child;
                child = runtime.Get(
                    child, key < runtime.Get(child, 12u) ? 0u : 8u);
            }
            runtime.allocate_key = key;
            Check(pal4::PalEventVolumnManager::InsertTreeNode(
                      state, child, parent, key, key + 100u,
                      runtime) == Address(key),
                  "insertion returns allocated node");
            inserted.insert(key);
            Validate(runtime, state, inserted);
            Check(runtime.Get(Address(key), 16u) == key + 100u,
                  "insertion copies mapped value");
        }
        runtime.allocate_key = 0u;
        Check(pal4::PalEventVolumnManager::InsertTreeNode(
                  state, Runtime::nil, Address(*inserted.rbegin()),
                  8u, 108u, runtime) == 0u &&
                  state.words[12u] == inserted.size(),
              "allocation failure leaves tree unchanged");
        Validate(runtime, state, inserted);
    }
}

void TestPublicInsertionPath() {
    Runtime runtime;
    runtime.Init();
    runtime.memory[Runtime::head + 4u] = Runtime::nil;
    InsertAdapter adapter(runtime);
    pal4::PalEventVolumnState state;
    state.words[10u] = Runtime::head;
    std::set<std::uint32_t> inserted;
    for (const auto key : {4u, 2u, 6u, 1u, 3u, 5u, 7u}) {
        runtime.allocate_key = key;
        Check(pal4::PalEventVolumnManager::InsertTreeEntry(
                  state, key, key + 100u, adapter) &&
                  adapter.last_insert == Address(key),
              "public insertion calls reconstructed node insertion");
        inserted.insert(key);
        Validate(runtime, state, inserted);
    }
    const auto count = state.words[12u];
    Check(!pal4::PalEventVolumnManager::InsertTreeEntry(
              state, 3u, 999u, adapter) &&
              state.words[12u] == count &&
              runtime.Get(Address(3u), 16u) == 103u,
          "public unique insertion preserves existing mapped value");
}

void TestRedNodeWithBlackSuccessor() {
    Runtime runtime;
    runtime.Init();
    runtime.memory[Runtime::head + 4u] = Runtime::nil;
    pal4::PalEventVolumnState state;
    state.words[10u] = Runtime::head;
    std::set<std::uint32_t> inserted;
    for (const auto key : {13u, 7u, 19u, 4u, 10u, 16u, 23u, 8u, 9u,
                           1u, 6u, 11u, 17u, 21u, 26u, 2u, 3u, 5u,
                           14u, 18u}) {
        auto child = runtime.Get(Runtime::head, 4u);
        auto parent = Runtime::head;
        while (child != Runtime::nil) {
            parent = child;
            child = runtime.Get(
                child, key < runtime.Get(child, 12u) ? 0u : 8u);
        }
        runtime.allocate_key = key;
        (void)pal4::PalEventVolumnManager::InsertTreeNode(
            state, child, parent, key, key + 100u, runtime);
        inserted.insert(key);
    }
    Validate(runtime, state, inserted);
    Check(runtime.Get(Address(13u), 20u) == 0u &&
              runtime.Get(Address(14u), 20u) == 1u,
          "red node has a black direct successor");
    Check(pal4::PalEventVolumnManager::EraseTreeNode(
              state, Address(13u), runtime) == Address(14u),
          "red-node deletion returns its successor");
    inserted.erase(13u);
    Validate(runtime, state, inserted);
}

}  // namespace

int main() {
    TestSevenNodeDeletion();
    TestRedSiblingRotation();
    TestMirroredRedSiblingRotation();
    TestNearRedChild();
    TestInsertionOrders();
    TestPublicInsertionPath();
    TestRedNodeWithBlackSuccessor();
}
