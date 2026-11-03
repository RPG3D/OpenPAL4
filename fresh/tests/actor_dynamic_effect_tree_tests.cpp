#include "pal4/actor_dynamic_effect_tree.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

constexpr std::uint32_t tree = 0x1000u;
constexpr std::uint32_t head = 0x2000u;
constexpr std::uint32_t nil = 0x3000u;
constexpr std::uint32_t first = 0x4000u;
constexpr std::uint32_t second = 0x5000u;

struct Runtime final : pal4::IActorDynamicEffectTreeRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {tree + 4u, head}, {tree + 12u, 0u},
        {head, head}, {head + 4u, nil}, {head + 8u, head},
        {head + 24u, 0u}, {nil, nil}, {nil + 8u, nil},
        {nil + 24u, 1u}};
    std::uint32_t shared_nil = nil;
    std::uint32_t references = 1u;
    std::uint32_t next_allocation = 0xa000u;
    bool fail_allocation = false;
    std::vector<std::string> trace;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        trace.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t SharedNil() override { return shared_nil; }
    std::uint32_t SharedReferences() override { return references; }
    void SetSharedNil(std::uint32_t value) override {
        shared_nil = value;
        trace.emplace_back("nil");
    }
    void SetSharedReferences(std::uint32_t value) override {
        references = value;
        trace.emplace_back("refs");
    }
    void ResetHandle(std::uint32_t address) override {
        trace.emplace_back("reset:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 28u, "node allocation size");
        if (fail_allocation) return 0u;
        const auto address = next_allocation;
        next_allocation += 0x40u;
        return address;
    }
    void Free(std::uint32_t address) override {
        trace.emplace_back("free:" + std::to_string(address));
    }
};

void SetNode(Runtime& runtime, std::uint32_t node, std::uint32_t parent,
             std::uint32_t left, std::uint32_t right,
             std::uint32_t color);

void EmptyTree() {
    Runtime runtime;
    pal4::ActorDynamicEffectTree::Destroy(tree, runtime);
    Check(runtime.words[tree + 4u] == 0u &&
              runtime.words[tree + 12u] == 0u, "empty tree fields");
    Check(runtime.references == 0u && runtime.shared_nil == 0u,
          "last owner releases shared nil");
    Check(runtime.trace == std::vector<std::string>{
              "free:8192", "write:4100", "write:4108", "refs", "nil",
              "free:12288"}, "empty tree release order");
}

void CountedTree() {
    Runtime runtime;
    runtime.words[tree + 12u] = 2u;
    runtime.words[head] = first;
    runtime.words[head + 4u] = first;
    runtime.words[head + 8u] = second;
    SetNode(runtime, first, head, nil, second, 1u);
    SetNode(runtime, second, first, nil, nil, 0u);
    runtime.references = 2u;
    pal4::ActorDynamicEffectTree::Destroy(tree, runtime);
    Check(runtime.words[head] == head && runtime.words[head + 4u] == nil &&
              runtime.words[head + 8u] == head, "bulk reset head");
    Check(runtime.references == 1u && runtime.shared_nil == nil,
          "shared nil remains for other owner");
    Check(runtime.trace[0] == "reset:20496" &&
              runtime.trace[1] == "free:20480" &&
              runtime.trace[2] == "reset:16400" &&
              runtime.trace[3] == "free:16384" &&
              runtime.trace[4] == "write:8196" &&
              runtime.trace[8] == "free:8192" &&
              runtime.trace.back() == "refs", "bulk clear order");
}

void UncountedNodes() {
    Runtime runtime;
    runtime.words[head] = first;
    runtime.words[head + 4u] = first;
    runtime.words[head + 8u] = second;
    runtime.words[nil + 24u] = 1u;
    SetNode(runtime, first, head, nil, second, 1u);
    SetNode(runtime, second, first, nil, nil, 0u);
    pal4::ActorDynamicEffectTree::Destroy(tree, runtime);
    Check(runtime.trace[0] == "write:20484" &&
              runtime.trace[1] == "write:8196" &&
              runtime.trace[2] == "write:8192" &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "reset:16400") <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "free:16384") &&
              std::find(runtime.trace.begin(), runtime.trace.end(),
                        "free:16384") <
                  std::find(runtime.trace.begin(), runtime.trace.end(),
                            "free:20480") &&
              runtime.words[tree + 12u] == 0u,
          "uncounted nodes erase before head release");
}

void NullSharedNil() {
    Runtime runtime;
    runtime.shared_nil = 0u;
    pal4::ActorDynamicEffectTree::Destroy(tree, runtime);
    Check(runtime.trace.back() == "nil", "null nil requires no free");
}

void SetNode(Runtime& runtime, std::uint32_t node, std::uint32_t parent,
             std::uint32_t left, std::uint32_t right,
             std::uint32_t color) {
    runtime.words[node] = left;
    runtime.words[node + 4u] = parent;
    runtime.words[node + 8u] = right;
    runtime.words[node + 24u] = color;
}

void CheckTree(const Runtime& runtime) {
    const auto root = runtime.words.at(head + 4u);
    if (root == nil) {
        Check(runtime.words.at(head) == head &&
                  runtime.words.at(head + 8u) == head &&
                  runtime.words.at(tree + 12u) == 0u, "empty erase result");
        return;
    }
    Check(runtime.words.at(root + 4u) == head &&
              runtime.words.at(root + 24u) == 1u, "black root parent");
    std::set<std::uint32_t> visited;
    std::vector<std::uint32_t> ordered;
    std::function<int(std::uint32_t, std::uint32_t)> visit =
        [&](std::uint32_t node, std::uint32_t parent) -> int {
        if (node == nil) return 1;
        Check(visited.insert(node).second &&
                  runtime.words.at(node + 4u) == parent,
              "acyclic parent links");
        const auto left = runtime.words.at(node);
        const auto right = runtime.words.at(node + 8u);
        if (runtime.words.at(node + 24u) == 0u)
            Check(runtime.words.at(left + 24u) == 1u &&
                      runtime.words.at(right + 24u) == 1u,
                  "red node has black children");
        const auto left_height = visit(left, node);
        ordered.push_back(node);
        const auto right_height = visit(right, node);
        Check(left_height == right_height, "black heights agree");
        return left_height + (runtime.words.at(node + 24u) == 1u ? 1 : 0);
    };
    (void)visit(root, head);
    Check(visited.size() == runtime.words.at(tree + 12u), "node count");
    Check(runtime.words.at(head) == ordered.front() &&
              runtime.words.at(head + 8u) == ordered.back(),
          "head extrema");
}

void EraseSingleton() {
    Runtime runtime;
    runtime.words[nil + 24u] = 1u;
    runtime.words[head] = first;
    runtime.words[head + 4u] = first;
    runtime.words[head + 8u] = first;
    runtime.words[tree + 12u] = 1u;
    SetNode(runtime, first, head, nil, nil, 1u);
    constexpr std::uint32_t result = 0x9000u;
    (void)pal4::ActorDynamicEffectTree::Erase(tree, result, first, runtime);
    CheckTree(runtime);
    Check(runtime.words[result] == head, "singleton successor");
    Check(runtime.trace[runtime.trace.size() - 4u] == "reset:16400" &&
              runtime.trace[runtime.trace.size() - 3u] == "free:16384",
          "singleton resets handle before freeing original node");
}

void EraseTwoChildren() {
    Runtime runtime;
    constexpr std::uint32_t right = 0x6000u;
    constexpr std::uint32_t result = 0x9000u;
    runtime.words[nil + 24u] = 1u;
    runtime.words[head] = first;
    runtime.words[head + 4u] = second;
    runtime.words[head + 8u] = right;
    runtime.words[tree + 12u] = 3u;
    SetNode(runtime, second, head, first, right, 1u);
    SetNode(runtime, first, second, nil, nil, 0u);
    SetNode(runtime, right, second, nil, nil, 0u);
    (void)pal4::ActorDynamicEffectTree::Erase(tree, result, second, runtime);
    CheckTree(runtime);
    Check(runtime.words[result] == right &&
              runtime.words[head + 4u] == right &&
              runtime.words[right] == first &&
              runtime.words[right + 24u] == 1u,
          "successor takes removed root position");
    Check(runtime.trace[runtime.trace.size() - 4u] == "reset:20496" &&
              runtime.trace[runtime.trace.size() - 3u] == "free:20480",
          "two-child erase releases requested node");
}

void EraseBlackLeavesWithRotations() {
    constexpr std::uint32_t result = 0x9000u;
    for (const bool mirror : {false, true}) {
        Runtime runtime;
        constexpr std::uint32_t low = 0x4000u;
        constexpr std::uint32_t middle = 0x5000u;
        constexpr std::uint32_t high = 0x6000u;
        constexpr std::uint32_t extra_low = 0x3800u;
        constexpr std::uint32_t extra_high = 0x7000u;
        runtime.words[nil + 24u] = 1u;
        runtime.words[tree + 12u] = 4u;
        runtime.words[head + 4u] = middle;
        SetNode(runtime, middle, head, low, high, 1u);
        SetNode(runtime, low, middle, mirror ? extra_low : nil, nil, 1u);
        SetNode(runtime, high, middle, nil, mirror ? nil : extra_high, 1u);
        if (mirror) {
            SetNode(runtime, extra_low, low, nil, nil, 0u);
            runtime.words[head] = extra_low;
            runtime.words[head + 8u] = high;
        } else {
            SetNode(runtime, extra_high, high, nil, nil, 0u);
            runtime.words[head] = low;
            runtime.words[head + 8u] = extra_high;
        }
        const auto removed = mirror ? high : low;
        (void)pal4::ActorDynamicEffectTree::Erase(
            tree, result, removed, runtime);
        CheckTree(runtime);
        Check(runtime.words[head + 4u] == (mirror ? low : high),
              "black leaf repairs with mirrored rotation");
    }
}

void EraseRangeBranches() {
    constexpr std::uint32_t result = 0x9000u;
    Runtime bulk;
    bulk.words[tree + 12u] = 2u;
    bulk.words[head] = first;
    bulk.words[head + 4u] = first;
    bulk.words[head + 8u] = second;
    SetNode(bulk, first, head, nil, second, 1u);
    SetNode(bulk, second, first, nil, nil, 0u);
    pal4::ActorDynamicEffectTree::EraseRange(
        tree, result, first, head, bulk);
    Check(bulk.trace.front() == "reset:20496" &&
              bulk.words[result] == head &&
              bulk.words[head + 4u] == nil &&
              bulk.words[tree + 12u] == 0u,
          "full range uses counted subtree clear");

    Runtime partial;
    partial.words[nil + 24u] = 1u;
    partial.words[head] = first;
    partial.words[head + 4u] = second;
    partial.words[head + 8u] = second;
    partial.words[tree + 12u] = 2u;
    SetNode(partial, second, head, first, nil, 1u);
    SetNode(partial, first, second, nil, nil, 0u);
    pal4::ActorDynamicEffectTree::EraseRange(
        tree, result, first, second, partial);
    CheckTree(partial);
    Check(partial.words[result] == second &&
              partial.words[head] == second &&
              std::find(partial.trace.begin(), partial.trace.end(),
                        "free:16384") != partial.trace.end(),
          "partial range deletes only first node");
}

void SuccessorBranches() {
    Runtime runtime;
    constexpr std::uint32_t low = 0x4000u;
    constexpr std::uint32_t middle = 0x5000u;
    constexpr std::uint32_t high = 0x6000u;
    constexpr std::uint32_t between = 0x5800u;
    runtime.words[head] = low;
    runtime.words[head + 4u] = middle;
    runtime.words[head + 8u] = high;
    SetNode(runtime, middle, head, low, high, 1u);
    SetNode(runtime, low, middle, nil, nil, 1u);
    SetNode(runtime, high, middle, between, nil, 1u);
    SetNode(runtime, between, high, nil, nil, 0u);
    Check(pal4::ActorDynamicEffectTree::Successor(middle, runtime) == between,
          "right subtree minimum");
    Check(pal4::ActorDynamicEffectTree::Successor(low, runtime) == middle,
          "left child ascends to parent");
    Check(pal4::ActorDynamicEffectTree::Successor(between, runtime) == high,
          "descendant ascends to next ancestor");
    Check(pal4::ActorDynamicEffectTree::Successor(high, runtime) == head,
          "maximum advances to head");
}

void ClearSubtreeOrder() {
    Runtime runtime;
    constexpr std::uint32_t left = 0x3800u;
    constexpr std::uint32_t right = 0x6000u;
    constexpr std::uint32_t right_left = 0x5800u;
    SetNode(runtime, first, head, left, right, 1u);
    SetNode(runtime, left, first, nil, nil, 1u);
    SetNode(runtime, right, first, right_left, nil, 1u);
    SetNode(runtime, right_left, right, nil, nil, 0u);
    pal4::ActorDynamicEffectTree::ClearSubtree(first, runtime);
    Check(runtime.trace == std::vector<std::string>{
              "reset:24592", "free:24576", "reset:22544",
              "free:22528", "reset:16400", "free:16384",
              "reset:14352", "free:14336"},
          "right branch is released before root and left spine");
    runtime.trace.clear();
    pal4::ActorDynamicEffectTree::ClearSubtree(nil, runtime);
    Check(runtime.trace.empty(), "nil subtree does nothing");
}

void InsertAndQuery() {
    Runtime runtime;
    constexpr std::uint32_t record = 0xb000u;
    const std::vector<std::int32_t> keys{5, -3, 10, 0, 8, 14, -7, 4, 6, 12};
    for (const auto key : keys) {
        runtime.words[record] = static_cast<std::uint32_t>(key);
        runtime.words[record + 4u] = static_cast<std::uint32_t>(key + 100);
        runtime.words[record + 8u] = 0xabcdefu;
        auto parent = head;
        auto cursor = runtime.words[head + 4u];
        while (cursor != nil) {
            parent = cursor;
            cursor = runtime.words[cursor +
                (key < static_cast<std::int32_t>(runtime.words[cursor + 12u])
                     ? 0u : 8u)];
        }
        const auto node = pal4::ActorDynamicEffectTree::InsertAt(
            tree, cursor, parent, record, runtime);
        Check(runtime.words[node + 12u] == static_cast<std::uint32_t>(key) &&
                  runtime.words[node + 16u] ==
                      static_cast<std::uint32_t>(key + 100) &&
                  runtime.words[node + 20u] == 0xabcdefu,
              "insertion copies key and effect handle");
        CheckTree(runtime);
        std::vector<std::int32_t> ordered;
        std::function<void(std::uint32_t)> visit = [&](std::uint32_t current) {
            if (current == nil) return;
            visit(runtime.words[current]);
            ordered.push_back(static_cast<std::int32_t>(
                runtime.words[current + 12u]));
            visit(runtime.words[current + 8u]);
        };
        visit(runtime.words[head + 4u]);
        Check(std::is_sorted(ordered.begin(), ordered.end()),
              "inserted tree uses signed key order");
    }
    Check(pal4::ActorDynamicEffectTree::Leftmost(
              runtime.words[head + 4u], runtime) == runtime.words[head],
          "leftmost root descendant");
    Check(pal4::ActorDynamicEffectTree::LowerBound(tree, -10, runtime) ==
              runtime.words[head], "lower bound before first");
    const auto found = pal4::ActorDynamicEffectTree::LowerBound(tree, 7, runtime);
    Check(static_cast<std::int32_t>(runtime.words[found + 12u]) == 8,
          "lower bound in key gap");
    Check(pal4::ActorDynamicEffectTree::LowerBound(tree, 20, runtime) == head,
          "lower bound after last");
    Check(pal4::ActorDynamicEffectTree::Predecessor(head, runtime) ==
              runtime.words[head + 8u], "head predecessor is maximum");
    Check(static_cast<std::int32_t>(runtime.words[
              pal4::ActorDynamicEffectTree::Predecessor(found, runtime) +
              12u]) == 6, "ordinary predecessor");

    for (const auto key : keys) {
        const auto node = pal4::ActorDynamicEffectTree::LowerBound(
            tree, key, runtime);
        (void)pal4::ActorDynamicEffectTree::Erase(tree, 0u, node, runtime);
        CheckTree(runtime);
    }
}

void InsertAllocationFailure() {
    Runtime runtime;
    runtime.fail_allocation = true;
    runtime.words[0xb000u] = 1u;
    bool caught = false;
    try {
        (void)pal4::ActorDynamicEffectTree::InsertAt(
            tree, nil, head, 0xb000u, runtime);
    } catch (const std::bad_alloc&) {
        caught = true;
    }
    Check(caught && runtime.words[tree + 12u] == 0u &&
              runtime.words[head + 4u] == nil,
          "allocation failure leaves tree untouched");
}

void RecordLifecycle() {
    Runtime runtime;
    constexpr std::uint32_t source = 0x8000u;
    constexpr std::uint32_t destination = 0x9000u;
    runtime.words[source] = static_cast<std::uint32_t>(-7);
    runtime.words[source + 4u] = 17u;
    runtime.words[source + 8u] = 29u;
    Check(pal4::ActorEffectRecord::Copy(0u, source, runtime) == 0u &&
              runtime.trace.empty(), "null destination skips record copy");
    Check(pal4::ActorEffectRecord::Copy(destination, source, runtime) ==
              destination &&
              runtime.words[destination] == static_cast<std::uint32_t>(-7) &&
              runtime.words[destination + 4u] == 17u &&
              runtime.words[destination + 8u] == 29u,
          "record copy retains key and both handle words");
    runtime.trace.clear();
    Check(pal4::ActorEffectRecord::Destroy(destination, 2u, runtime) ==
              destination &&
              runtime.trace == std::vector<std::string>{"reset:36864"},
          "nondeleting record destructor only resets handle");
    runtime.trace.clear();
    Check(pal4::ActorEffectRecord::Destroy(destination, 3u, runtime) ==
              destination &&
              runtime.trace == std::vector<std::string>{
                  "reset:36864", "free:36864"},
          "deleting record destructor resets before freeing");
}

void MixedTreeOrders() {
    constexpr std::uint32_t record = 0xb000u;
    for (int rotation = 0; rotation < 8; ++rotation) {
        Runtime runtime;
        std::vector<std::int32_t> keys{
            -13, -8, -3, 0, 2, 5, 7, 9, 11, 16, 20, 24};
        std::rotate(keys.begin(), keys.begin() + rotation, keys.end());
        if (rotation % 2 != 0) std::reverse(keys.begin(), keys.end());
        for (const auto key : keys) {
            runtime.words[record] = static_cast<std::uint32_t>(key);
            runtime.words[record + 4u] = 0u;
            runtime.words[record + 8u] = 0u;
            auto parent = head;
            auto cursor = runtime.words[head + 4u];
            while (cursor != nil) {
                parent = cursor;
                cursor = runtime.words[cursor +
                    (key < static_cast<std::int32_t>(
                         runtime.words[cursor + 12u]) ? 0u : 8u)];
            }
            (void)pal4::ActorDynamicEffectTree::InsertAt(
                tree, cursor, parent, record, runtime);
            CheckTree(runtime);
        }
        for (const auto key : keys) {
            const auto node = pal4::ActorDynamicEffectTree::LowerBound(
                tree, key, runtime);
            Check(node != head &&
                      static_cast<std::int32_t>(
                          runtime.words[node + 12u]) == key,
                  "mixed order key remains findable");
            (void)pal4::ActorDynamicEffectTree::Erase(
                tree, 0u, node, runtime);
            CheckTree(runtime);
        }
    }
}

}  // namespace

int main() {
    try {
        EmptyTree();
        CountedTree();
        UncountedNodes();
        NullSharedNil();
        EraseSingleton();
        EraseTwoChildren();
        EraseBlackLeavesWithRotations();
        EraseRangeBranches();
        SuccessorBranches();
        ClearSubtreeOrder();
        InsertAndQuery();
        InsertAllocationFailure();
        RecordLifecycle();
        MixedTreeOrders();
        std::cout << "actor_dynamic_effect_tree_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
