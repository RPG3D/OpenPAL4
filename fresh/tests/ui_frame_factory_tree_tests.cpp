#include "pal4/ui_frame_registry.h"
#include "pal4/ui_red_black_tree_remove.h"
#include "pal4/ui_tree_pair_data.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameFactoryTreeRuntime,
                       pal4::IUiRedBlackTreeRemoveRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t predecessor = 0x6200u;
    std::uint32_t predecessor_source = 0x6100u;
    std::uint32_t insert_result = 0x7400u;
    std::uint32_t inserted_parent = 0;
    std::uint32_t inserted_key = 0;
    std::uint32_t inserted_creator = 0;
    bool throw_registration = false;
    std::uint32_t next_allocation = 0x7000u;
    std::uint32_t allocation_size = 0u;
    std::vector<std::uint32_t> freed_nodes;

    Runtime() {
        words[0x5008u] = 0x6000u;
        words[0x6000u] = 0x6000u;
        words[0x6004u] = 0xdeadu;
        words[0x6008u] = 0x6000u;
        words[0x5010u] = 0u;
        words[0xdec1u] = 1u;
        bytes[0x500cu] = 0u;
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read word");
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        events.emplace_back("read byte");
        return bytes.at(address);
    }
    std::uint32_t SharedNil() override {
        events.emplace_back("nil");
        return 0xdeadu;
    }
    std::uint32_t Predecessor(std::uint32_t node) override {
        events.emplace_back("predecessor");
        Check(node == predecessor_source,
              "predecessor requested for last parent");
        return predecessor;
    }
    std::uint32_t InsertFactoryNode(
        std::uint32_t tree, std::uint32_t nil,
        std::uint32_t parent, std::uint32_t key,
        std::uint32_t creator) override {
        events.emplace_back("insert");
        Check(tree == 0x5004u && nil == 0xdeadu,
              "insertion tree and shared nil");
        inserted_parent = parent;
        inserted_key = key;
        inserted_creator = creator;
        return insert_result;
    }
    std::uint32_t SingletonManager() override {
        events.emplace_back("singleton manager");
        return 0x4400u;
    }
    std::uint32_t RegisterFactorySingleton(
        std::uint32_t manager, const char* type_name,
        std::uint32_t id, std::uint32_t constructor,
        std::uint32_t type_info) override {
        events.emplace_back("register singleton");
        Check(manager == 0x4400u &&
                  std::string_view(type_name) ==
                      "fsObjectFactory < template-arg4 >" &&
                  id == 1019u && constructor == 0x4c1c10u &&
                  type_info == 0x4c2210u,
              "singleton identity matches frame registration");
        if (throw_registration)
            throw std::runtime_error("registration failure");
        return 0x5000u;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.emplace_back("write word");
        words[address] = value;
    }
    std::string_view FrameName(std::uint32_t) override {
        throw std::logic_error("unexpected frame name");
    }
    std::uint32_t FindFactoryNode(std::uint32_t,
                                  const char*) override {
        throw std::logic_error("unexpected factory find");
    }
    std::uint32_t FactoryEndNode(std::uint32_t) override {
        throw std::logic_error("unexpected factory end");
    }
    std::uint32_t CreateFrame(std::uint32_t,
                              const char*) override {
        throw std::logic_error("unexpected frame creation");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        events.emplace_back("allocate");
        allocation_size = bytes;
        const auto result = next_allocation;
        next_allocation += 0x20u;
        return result;
    }
    void Free(std::uint32_t) override {
        throw std::logic_error("unexpected free");
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    void FreeMemory(std::uint32_t address) override {
        freed_nodes.push_back(address);
    }
};

void ConfigureRoot(Runtime& runtime, std::uint32_t root_key) {
    runtime.words[0x6004u] = 0x6100u;
    runtime.words[0x610cu] = root_key;
    runtime.words[0x6100u] = 0xdeadu;
    runtime.words[0x6108u] = 0xdeadu;
    runtime.words[0x6000u] = 0x6100u;
}

void CheckEmptyTree() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t cached = 0;
    Check(pal4::UiFrameFactoryTree::InsertRegistration(
              0x8100u, 0x4200u, guard, cached, runtime) == 0x7400u,
          "empty tree forwards insertion result");
    Check(guard == 0x81u && cached == 0x5000u,
          "singleton bit set while preserving other guard bits");
    Check(runtime.inserted_parent == 0x6000u &&
              runtime.inserted_key == 0x8100u &&
              runtime.inserted_creator == 0x4200u,
          "empty tree inserts key and creator under head");
}

void CheckUnsignedBranches() {
    Runtime lower;
    ConfigureRoot(lower, 0x80000000u);
    std::uint8_t guard = 1u;
    std::uint32_t cached = 0x5000u;
    Check(pal4::UiFrameFactoryTree::InsertRegistration(
              0x7fffffffu, 0x4200u, guard, cached, lower) == 0x7400u &&
              lower.inserted_parent == 0x6100u,
          "unsigned smaller key takes left branch and inserts");

    Runtime higher;
    ConfigureRoot(higher, 0x80000000u);
    Check(pal4::UiFrameFactoryTree::InsertRegistration(
              0xf0000000u, 0x4200u, guard, cached, higher) == 0x7400u &&
              higher.inserted_parent == 0x6100u,
          "unsigned greater key takes right branch and inserts");
    Check(higher.events.front() == "read word",
          "cached singleton avoids new registration");
}

void CheckDuplicatePolicy() {
    Runtime unique;
    ConfigureRoot(unique, 100u);
    std::uint8_t guard = 1u;
    std::uint32_t cached = 0x5000u;
    Check(pal4::UiFrameFactoryTree::InsertRegistration(
              100u, 0x4200u, guard, cached, unique) == 100u &&
              unique.inserted_parent == 0u,
          "unique tree returns duplicate key without insertion");

    Runtime duplicates;
    ConfigureRoot(duplicates, 100u);
    duplicates.bytes[0x500cu] = 1u;
    Check(pal4::UiFrameFactoryTree::InsertRegistration(
              100u, 0x4200u, guard, cached, duplicates) == 0x7400u &&
              duplicates.inserted_parent == 0x6100u,
          "duplicate-enabled tree inserts an equal key");
}

void CheckPredecessor() {
    Runtime runtime;
    ConfigureRoot(runtime, 50u);
    runtime.words[0x6108u] = 0x6300u;
    runtime.words[0x630cu] = 80u;
    runtime.words[0x6300u] = 0xdeadu;
    runtime.words[0x6308u] = 0xdeadu;
    runtime.predecessor = 0x6100u;
    runtime.predecessor_source = 0x6300u;
    std::uint8_t guard = 1u;
    std::uint32_t cached = 0x5000u;
    Check(pal4::UiFrameFactoryTree::InsertRegistration(
              70u, 0x4200u, guard, cached, runtime) == 0x7400u &&
              runtime.inserted_parent == 0x6300u,
          "left descent tests predecessor before insertion");
    Check(std::find(runtime.events.begin(), runtime.events.end(),
                    "predecessor") != runtime.events.end(),
          "predecessor helper called");
}

void CheckRegistrationException() {
    Runtime runtime;
    runtime.throw_registration = true;
    std::uint8_t guard = 0u;
    std::uint32_t cached = 0u;
    bool threw = false;
    try {
        (void)pal4::UiFrameFactoryTree::InsertRegistration(
            1u, 2u, guard, cached, runtime);
    } catch (const std::runtime_error& error) {
        threw = std::string_view(error.what()) == "registration failure";
    }
    Check(threw && guard == 1u && cached == 0u,
          "singleton guard set before registration exception");
}

void CheckFindNode() {
    Runtime runtime;
    constexpr std::uint32_t output = 0x9000u;
    runtime.words[output] = 0xffffffffu;
    Check(pal4::UiFrameFactoryTree::FindNode(
              0x5004u, output, 42u, runtime) == output &&
              runtime.words[output] == 0x6000u,
          "empty tree returns output and stores head");

    ConfigureRoot(runtime, 0x80000000u);
    runtime.words[0x6100u] = 0x6200u;
    runtime.words[0x6108u] = 0x6300u;
    runtime.words[0x620cu] = 10u;
    runtime.words[0x6200u] = 0xdeadu;
    runtime.words[0x6208u] = 0xdeadu;
    runtime.words[0x630cu] = 0xf0000000u;
    runtime.words[0x6300u] = 0xdeadu;
    runtime.words[0x6308u] = 0xdeadu;
    for (const auto [key, expected] :
         {std::pair{10u, 0x6200u},
          std::pair{0x80000000u, 0x6100u},
          std::pair{0xf0000000u, 0x6300u},
          std::pair{9u, 0x6000u},
          std::pair{11u, 0x6000u},
          std::pair{0x7fffffffu, 0x6000u},
          std::pair{0x80000001u, 0x6000u},
          std::pair{0xffffffffu, 0x6000u}}) {
        runtime.words[output] = 0xffffffffu;
        Check(pal4::UiFrameFactoryTree::FindNode(
                  0x5004u, output, key, runtime) == output &&
                  runtime.words[output] == expected,
              "exact unsigned key only, with both traversal directions");
    }
}

int CheckSubtree(const Runtime& runtime, const std::uint32_t node,
                 const std::uint32_t parent,
                 std::vector<std::uint32_t>& ordered) {
    if (node == 0xdeadu) return 1;
    const auto& words = runtime.words;
    Check(words.at(node + 4u) == parent,
          "each child points back to its parent");
    const auto left = words.at(node);
    const auto right = words.at(node + 8u);
    const auto colour = words.at(node + 20u);
    Check(colour <= 1u, "node colour is red or black");
    if (colour == 0u) {
        Check(words.at(left + 20u) == 1u &&
                  words.at(right + 20u) == 1u,
              "red node has black children");
    }
    const int left_height = CheckSubtree(runtime, left, node, ordered);
    ordered.push_back(words.at(node + 12u));
    const int right_height = CheckSubtree(runtime, right, node, ordered);
    Check(left_height == right_height,
          "both branches have equal black height");
    return left_height + static_cast<int>(colour);
}

void CheckInsertedTree(const Runtime& runtime,
                       const std::vector<std::uint32_t>& expected) {
    const auto& words = runtime.words;
    const auto root = words.at(0x6004u);
    if (expected.empty()) {
        Check(root == 0xdeadu && words.at(0x5010u) == 0u &&
                  words.at(0x6000u) == 0x6000u &&
                  words.at(0x6008u) == 0x6000u,
              "empty factory tree restores nil root and head extrema");
        return;
    }
    Check(root != 0xdeadu && words.at(root + 4u) == 0x6000u &&
              words.at(root + 20u) == 1u,
          "root has head parent and is black");
    Check(words.at(0x5010u) == expected.size(), "tree count");
    std::vector<std::uint32_t> actual;
    (void)CheckSubtree(runtime, root, 0x6000u, actual);
    auto sorted = expected;
    std::sort(sorted.begin(), sorted.end());
    Check(actual == sorted, "in-order unsigned keys");
    Check(words.at(words.at(0x6000u) + 12u) == sorted.front() &&
              words.at(words.at(0x6008u) + 12u) == sorted.back(),
          "head tracks minimum and maximum");
}

std::uint32_t FindNodeByKey(const Runtime& runtime,
                            const std::uint32_t key) {
    auto node = runtime.words.at(0x6004u);
    while (node != 0xdeadu) {
        const auto current = runtime.words.at(node + 12u);
        if (current == key) return node;
        node = runtime.words.at(node + (key < current ? 0u : 8u));
    }
    throw std::runtime_error("key missing before deletion");
}

void CheckNodeDeletion() {
    std::array<std::uint32_t, 5> removal_order{40u, 20u, 60u, 50u, 70u};
    std::sort(removal_order.begin(), removal_order.end());
    do {
        Runtime runtime;
        std::vector<std::uint32_t> expected;
        for (const auto key : std::array{40u, 20u, 60u, 50u, 70u}) {
            runtime.words[0x8000u] = key;
            runtime.words[0x8004u] = key + 100u;
            auto parent = 0x6000u;
            auto cursor = runtime.words.at(0x6004u);
            while (cursor != 0xdeadu) {
                parent = cursor;
                cursor = runtime.words.at(
                    cursor + (key < runtime.words.at(cursor + 12u)
                                  ? 0u : 8u));
            }
            (void)pal4::UiFrameFactoryTree::InsertNode(
                0x5004u, 0x9000u, 0xdeadu, parent, 0x8000u, runtime);
            expected.push_back(key);
        }
        for (const auto key : removal_order) {
            const auto removed = FindNodeByKey(runtime, key);
            const auto old_size = expected.size();
            Check(pal4::UiRedBlackTreeRemove::RemoveFactory(
                      0x5004u, 0x9000u, removed, runtime) == 0x9000u,
                  "factory delete returns output slot");
            Check(runtime.freed_nodes.back() == removed &&
                      runtime.freed_nodes.size() == 6u - old_size,
                  "removed factory node is freed once");
            expected.erase(std::find(expected.begin(), expected.end(), key));
            CheckInsertedTree(runtime, expected);
            if (expected.empty()) {
                Check(runtime.words.at(0x9000u) == 0x6000u,
                      "last deletion returns head iterator");
            } else {
                auto sorted = expected;
                std::sort(sorted.begin(), sorted.end());
                const auto next = std::upper_bound(
                    sorted.begin(), sorted.end(), key);
                if (next == sorted.end()) {
                    Check(runtime.words.at(0x9000u) == 0x6000u,
                          "maximum deletion returns head iterator");
                } else {
                    Check(runtime.words.at(runtime.words.at(0x9000u) + 12u) ==
                              *next,
                          "deletion returns next in-order node");
                }
            }
        }
    } while (std::next_permutation(removal_order.begin(),
                                   removal_order.end()));
}

void CheckNodeInsertion() {
    std::array<std::uint32_t, 5> keys{
        0x80000000u, 1u, 0xffffffffu, 40u, 0x7fffffffu};
    std::sort(keys.begin(), keys.end());
    do {
        Runtime runtime;
        std::vector<std::uint32_t> expected;
        for (const auto key : keys) {
            runtime.words[0x8000u] = key;
            runtime.words[0x8004u] = key ^ 0xa5a5a5a5u;
            auto parent = 0x6000u;
            auto cursor = runtime.words.at(0x6004u);
            while (cursor != 0xdeadu) {
                parent = cursor;
                cursor = runtime.words.at(
                    cursor + (key < runtime.words.at(cursor + 12u)
                                  ? 0u : 8u));
            }
            Check(pal4::UiFrameFactoryTree::InsertNode(
                      0x5004u, 0x9000u, 0xdeadu, parent, 0x8000u,
                      runtime) == 0x9000u,
                  "insertion returns the output slot");
            const auto inserted = runtime.words.at(0x9000u);
            Check(runtime.allocation_size == 0x18u &&
                      runtime.words.at(inserted + 12u) == key &&
                      runtime.words.at(inserted + 16u) ==
                          (key ^ 0xa5a5a5a5u),
                  "24-byte node and both copied data words");
            expected.push_back(key);
            CheckInsertedTree(runtime, expected);
        }
    } while (std::next_permutation(keys.begin(), keys.end()));

    Runtime duplicates;
    duplicates.words[0x8000u] = 20u;
    duplicates.words[0x8004u] = 1u;
    (void)pal4::UiFrameFactoryTree::InsertNode(
        0x5004u, 0x9000u, 0xdeadu, 0x6000u, 0x8000u, duplicates);
    (void)pal4::UiFrameFactoryTree::InsertNode(
        0x5004u, 0x9000u, 0xdeadu, 0x7000u, 0x8000u, duplicates);
    CheckInsertedTree(duplicates, {20u, 20u});

    Runtime hinted;
    hinted.words[0x8000u] = 30u;
    hinted.words[0x8004u] = 1u;
    (void)pal4::UiFrameFactoryTree::InsertNode(
        0x5004u, 0x9000u, 0xdeadu, 0x6000u, 0x8000u, hinted);
    hinted.words[0x8000u] = 40u;
    (void)pal4::UiFrameFactoryTree::InsertNode(
        0x5004u, 0x9000u, 0x6000u, 0x7000u, 0x8000u, hinted);
    Check(hinted.words.at(0x7000u) == 0x7020u,
          "non-nil child hint forces left linkage");
}

struct StorageRuntime final : pal4::IUiFrameFactoryTreeStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> allocations{
        0x5000u, 0x6000u, 0x7000u, 0x8000u, 0x9000u, 0xa000u};
    std::vector<std::uint32_t> sizes;
    std::vector<std::uint32_t> freed;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> deleted;
    std::map<std::uint32_t, std::uint32_t> successors;
    std::uint32_t nil = 0u;
    std::uint32_t users = 0u;
    std::size_t next = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        sizes.push_back(size);
        return allocations.at(next++);
    }
    void Free(std::uint32_t address) override {
        calls.emplace_back("free " + std::to_string(address));
        freed.push_back(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override { nil = address; }
    std::uint32_t SharedNilUsers() override { return users; }
    void SetSharedNilUsers(std::uint32_t count) override { users = count; }
    void GetFirstElement(std::uint32_t) override {
        calls.emplace_back("first");
    }
    std::uint32_t NextNode(std::uint32_t node) override {
        calls.emplace_back("next " + std::to_string(node));
        return successors.at(node);
    }
    void DeleteNode(std::uint32_t, std::uint32_t node) override {
        calls.emplace_back("delete " + std::to_string(node));
        deleted.push_back(node);
    }
    std::uint32_t InvokeDeletingDestructor(std::uint32_t object,
                                           std::uint32_t flag) override {
        calls.emplace_back("virtual delete " + std::to_string(object) +
                           ":" + std::to_string(flag));
        return 0xabcdu;
    }
};

void CheckStorageConstruction() {
    StorageRuntime failed;
    failed.allocations[0] = 0u;
    Check(pal4::UiFrameFactoryTreeStorage::Construct(
              0x5au, failed) == 0u &&
              failed.sizes == std::vector<std::uint32_t>{0x14u} &&
              failed.nil == 0u && failed.users == 0u &&
              failed.words.empty() && failed.bytes.empty(),
          "first allocation failure has no tree side effects");

    StorageRuntime runtime;
    Check(pal4::UiFrameFactoryTreeStorage::Construct(
              0x5au, runtime) == 0x5000u,
          "first tree allocation returned");
    Check(runtime.sizes ==
              (std::vector<std::uint32_t>{0x14u, 0x18u, 0x18u}) &&
              runtime.nil == 0x6000u && runtime.users == 1u &&
              runtime.freed.empty(),
          "new shared nil is retained and counted");
    Check(runtime.bytes.at(0x5004u) == 0x5au &&
              runtime.bytes.at(0x5005u) == 0x5au &&
              runtime.bytes.at(0x500cu) == 0u &&
              runtime.words.at(0x5000u) == 0x84444cu &&
              runtime.words.at(0x5008u) == 0x7000u &&
              runtime.words.at(0x5010u) == 0u,
          "seed, vtable, head and count fields");
    Check(runtime.words.at(0x6000u) == 0u &&
              runtime.words.at(0x6004u) == 0u &&
              runtime.words.at(0x6008u) == 0u &&
              runtime.words.at(0x6014u) == 1u &&
              runtime.words.at(0x7000u) == 0x7000u &&
              runtime.words.at(0x7004u) == 0x6000u &&
              runtime.words.at(0x7008u) == 0x7000u &&
              runtime.words.at(0x7014u) == 0u,
          "nil and empty-head links and colours");

    Check(pal4::UiFrameFactoryTreeStorage::Construct(
              0xa5u, runtime) == 0x8000u &&
              runtime.sizes == (std::vector<std::uint32_t>{
                  0x14u, 0x18u, 0x18u, 0x14u, 0x18u, 0x18u}) &&
              runtime.nil == 0x6000u && runtime.users == 2u &&
              runtime.freed == std::vector<std::uint32_t>{0x9000u},
          "second tree discards candidate and reuses shared nil");
    Check(runtime.bytes.at(0x8004u) == 0xa5u &&
              runtime.bytes.at(0x8005u) == 0xa5u &&
              runtime.words.at(0x8008u) == 0xa000u &&
              runtime.words.at(0xa004u) == 0x6000u &&
              runtime.words.at(0xa000u) == 0xa000u &&
              runtime.words.at(0xa008u) == 0xa000u &&
              runtime.words.at(0x6014u) == 1u,
          "reused nil remains intact and new head is self-linked");
}

void CheckStorageDestruction() {
    StorageRuntime runtime;
    (void)pal4::UiFrameFactoryTreeStorage::Construct(0u, runtime);
    (void)pal4::UiFrameFactoryTreeStorage::Construct(0u, runtime);
    runtime.words[0x7000u] = 0x7100u;
    runtime.words[0x7004u] = 0x7100u;
    runtime.words[0x7008u] = 0x7100u;
    runtime.words[0x5010u] = 1u;
    runtime.words[0x7100u] = 0x6000u;
    runtime.words[0x7108u] = 0x6000u;
    pal4::UiFrameFactoryTreeStorage::Destroy(0x5000u, runtime);
    Check(runtime.freed == (std::vector<std::uint32_t>{
              0x9000u, 0x7100u, 0x7000u}) &&
              runtime.calls.back() == "free 28672" &&
              std::find(runtime.calls.begin(), runtime.calls.end(),
                        "first") != runtime.calls.end(),
          "nonempty tree bulk frees before head release");
    Check(runtime.words.at(0x5000u) == 0x84444cu &&
              runtime.words.at(0x5008u) == 0u &&
              runtime.words.at(0x5010u) == 0u &&
              runtime.words.at(0x7000u) == 0x7000u &&
              runtime.words.at(0x7004u) == 0x6000u &&
              runtime.words.at(0x7008u) == 0x7000u &&
              runtime.nil == 0x6000u && runtime.users == 1u,
          "bulk path resets head and preserves nil with one user");

    runtime.calls.clear();
    pal4::UiFrameFactoryTreeStorage::Destroy(0x8000u, runtime);
    Check(runtime.nil == 0u && runtime.users == 0u &&
              runtime.freed == (std::vector<std::uint32_t>{
                  0x9000u, 0x7100u, 0x7000u, 0xa000u, 0x6000u}) &&
              runtime.calls == (std::vector<std::string>{
                  "free 40960", "free 24576"}),
          "last tree releases head then shared nil");

    StorageRuntime inconsistent;
    inconsistent.nil = 0x6000u;
    inconsistent.users = 1u;
    inconsistent.words[0x5008u] = 0x7000u;
    inconsistent.words[0x5010u] = 0u;
    inconsistent.words[0x7000u] = 0x7100u;
    inconsistent.successors[0x7100u] = 0x7200u;
    inconsistent.successors[0x7200u] = 0x7000u;
    pal4::UiFrameFactoryTreeStorage::Destroy(0x5000u, inconsistent);
    Check(inconsistent.deleted ==
              (std::vector<std::uint32_t>{0x7100u, 0x7200u}) &&
              inconsistent.calls == (std::vector<std::string>{
                  "next 28928", "delete 28928", "next 29184",
                  "delete 29184", "free 28672", "free 24576"}),
          "zero-count linked nodes use iterator before each deletion");
}

void CheckFreeNodes() {
    StorageRuntime runtime;
    runtime.nil = 0x6000u;
    Check(pal4::UiFrameFactoryTreeStorage::FreeNodes(
              0x6000u, runtime) == 0x6000u && runtime.freed.empty(),
          "shared nil is a no-op");
    runtime.words[0x7100u] = 0x7200u;
    runtime.words[0x7108u] = 0x7300u;
    runtime.words[0x7200u] = 0x6000u;
    runtime.words[0x7208u] = 0x6000u;
    runtime.words[0x7300u] = 0x6000u;
    runtime.words[0x7308u] = 0x6000u;
    Check(pal4::UiFrameFactoryTreeStorage::FreeNodes(
              0x7100u, runtime) == 0x6000u &&
              runtime.freed == (std::vector<std::uint32_t>{
                  0x7300u, 0x7100u, 0x7200u}),
          "right subtree is freed before node and left chain");
}

void CheckDeleteIfPresent() {
    StorageRuntime runtime;
    Check(!pal4::UiFrameFactoryTreeStorage::DeleteIfPresent(
              0u, runtime).has_value() && runtime.calls.empty(),
          "null factory object does not dispatch or define a return");
    Check(pal4::UiFrameFactoryTreeStorage::DeleteIfPresent(
              0x5000u, runtime) == 0xabcdu,
          "non-null branch forwards virtual return value");
    Check(runtime.calls ==
              std::vector<std::string>{"virtual delete 20480:1"},
          "non-null factory object invokes first virtual slot with flag one");
}

void CheckPairDataCopy() {
    Runtime runtime;
    runtime.words[0x8000u] = 0x12345678u;
    runtime.words[0x8004u] = 0xabcdef01u;
    Check(pal4::UiTreePairData::Copy(0u, 0xdeadbeefu, runtime) == 0u,
          "null destination does not dereference source");
    Check(pal4::UiTreePairData::Copy(
              0x8100u, 0x8000u, runtime) == 0x8100u &&
              runtime.words.at(0x8100u) == 0x12345678u &&
              runtime.words.at(0x8104u) == 0xabcdef01u,
          "copies exactly two words and returns destination");
    Check(pal4::UiTreePairData::Copy(
              0x8004u, 0x8000u, runtime) == 0x8004u &&
              runtime.words.at(0x8004u) == 0x12345678u &&
              runtime.words.at(0x8008u) == 0x12345678u,
          "overlap observes first write before second source read");
}

}  // namespace

int main() {
    try {
        CheckEmptyTree();
        CheckUnsignedBranches();
        CheckDuplicatePolicy();
        CheckPredecessor();
        CheckRegistrationException();
        CheckFindNode();
        CheckNodeInsertion();
        CheckNodeDeletion();
        CheckStorageConstruction();
        CheckStorageDestruction();
        CheckFreeNodes();
        CheckDeleteIfPresent();
        CheckPairDataCopy();
        std::cout << "ui_frame_factory_tree_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
