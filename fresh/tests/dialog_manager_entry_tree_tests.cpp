#include "pal4/dialog_manager_entry_tree.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class TreeRuntime final : public pal4::IDialogManagerEntryTreeEraseRuntime {
public:
    static constexpr std::uint32_t kNil = 0x9000;
    static constexpr std::uint32_t kHead = 0x1000;

    TreeRuntime() {
        nodes[kNil] = {kNil, kNil, kNil, 0, 1};
        nodes[kHead] = {kHead, kNil, kHead, 0, 0};
    }

    std::map<std::uint32_t, std::array<std::uint32_t, 5>> nodes;
    std::uint32_t next_node = 0x2000;
    std::size_t freed_nodes = 0;

    std::uint32_t NilNode() override { return kNil; }
    std::uint32_t NodeWord(std::uint32_t node,
                           std::size_t offset) override {
        return nodes.at(node).at(offset / 4);
    }
    void SetNodeWord(std::uint32_t node, std::size_t offset,
                     std::uint32_t value) override {
        nodes.at(node).at(offset / 4) = value;
    }
    std::uint32_t AllocateNode(std::uint32_t bytes) override {
        Check(bytes == 0x14, "node allocation size");
        const auto node = next_node;
        next_node += 0x20;
        nodes[node] = {0, 0, 0, 0, 0};
        return node;
    }
    void FreeNode(std::uint32_t node) override {
        Check(node != kNil && node != kHead, "only an allocated node is freed");
        Check(nodes.erase(node) == 1, "node is freed exactly once");
        ++freed_nodes;
    }
};

int CheckSubtree(TreeRuntime& runtime, std::uint32_t node,
                 std::uint32_t parent, std::vector<std::uint32_t>& keys) {
    if (node == TreeRuntime::kNil) return 1;
    Check(runtime.NodeWord(node, 4) == parent, "parent link");
    const auto left = runtime.NodeWord(node, 0);
    const auto right = runtime.NodeWord(node, 8);
    if (runtime.NodeWord(node, 16) == 0) {
        Check(runtime.NodeWord(left, 16) == 1, "red left child");
        Check(runtime.NodeWord(right, 16) == 1, "red right child");
    }
    const auto left_depth = CheckSubtree(runtime, left, node, keys);
    keys.push_back(runtime.NodeWord(node, 12));
    const auto right_depth = CheckSubtree(runtime, right, node, keys);
    Check(left_depth == right_depth, "black height");
    return left_depth + static_cast<int>(runtime.NodeWord(node, 16));
}

void CheckTree(pal4::DialogManagerState& state, TreeRuntime& runtime,
               const std::vector<std::uint32_t>& expected) {
    const auto root = runtime.NodeWord(TreeRuntime::kHead, 4);
    if (expected.empty()) {
        Check(root == TreeRuntime::kNil &&
                  runtime.NodeWord(TreeRuntime::kHead, 0) ==
                      TreeRuntime::kHead &&
                  runtime.NodeWord(TreeRuntime::kHead, 8) ==
                      TreeRuntime::kHead &&
                  state.Word(12) == 0,
              "empty tree header and count");
        return;
    }
    Check(runtime.NodeWord(root, 16) == 1, "black root");
    std::vector<std::uint32_t> keys;
    static_cast<void>(CheckSubtree(runtime, root, TreeRuntime::kHead, keys));
    Check(keys == expected, "inorder keys");
    Check(state.Word(12) == expected.size(), "tree count");
    auto minimum = root;
    while (runtime.NodeWord(minimum, 0) != TreeRuntime::kNil)
        minimum = runtime.NodeWord(minimum, 0);
    auto maximum = root;
    while (runtime.NodeWord(maximum, 8) != TreeRuntime::kNil)
        maximum = runtime.NodeWord(maximum, 8);
    Check(runtime.NodeWord(TreeRuntime::kHead, 0) == minimum,
          "head minimum");
    Check(runtime.NodeWord(TreeRuntime::kHead, 8) == maximum,
          "head maximum");
}

std::uint32_t FindNode(TreeRuntime& runtime, const std::uint32_t key) {
    auto node = runtime.NodeWord(TreeRuntime::kHead, 4);
    while (node != TreeRuntime::kNil) {
        const auto current = runtime.NodeWord(node, 12);
        if (current == key) return node;
        node = runtime.NodeWord(node, key < current ? 0 : 8);
    }
    throw std::runtime_error("expected key missing from tree");
}

void TestExtendedTree() {
    const std::array<std::vector<std::uint32_t>, 6> cases{{
        {10, 5, 1}, {10, 15, 20}, {10, 5, 7},
        {10, 15, 12}, {10, 5, 15, 1, 7}, {10, 5, 15, 20, 12}}};
    for (const auto& sequence : cases) {
        pal4::DialogManagerState state;
        state.SetWord(4, TreeRuntime::kHead);
        state.SetByte(8, 1);
        TreeRuntime runtime;
        std::vector<std::uint32_t> expected;
        for (const auto key : sequence) {
            const auto result = pal4::DialogManagerEntryTree::Insert(
                state, key, runtime);
            Check(result.inserted && result.node != 0, "extended insert");
            expected.push_back(key);
            std::sort(expected.begin(), expected.end());
            CheckTree(state, runtime, expected);
        }
        const auto duplicate = pal4::DialogManagerEntryTree::Insert(
            state, sequence.front(), runtime);
        Check(duplicate.inserted, "extended tree permits duplicate keys");
        expected.push_back(sequence.front());
        std::sort(expected.begin(), expected.end());
        CheckTree(state, runtime, expected);
    }
}

void TestStandardTree() {
    pal4::DialogManagerState state;
    state.SetWord(4, TreeRuntime::kHead);
    TreeRuntime runtime;
    auto result = pal4::DialogManagerEntryTree::Insert(state, 10, runtime);
    Check(result.inserted && result.node == FindNode(runtime, 10),
          "empty standard tree inserts root");
    CheckTree(state, runtime, {10});
    result = pal4::DialogManagerEntryTree::Insert(state, 10, runtime);
    Check(!result.inserted && result.node == FindNode(runtime, 10),
          "standard duplicate returns existing node");
    result = pal4::DialogManagerEntryTree::Insert(state, 11, runtime);
    Check(result.inserted && result.node == FindNode(runtime, 11),
          "standard larger key inserts right leaf");
    Check(pal4::DialogManagerEntryTree::Insert(state, 5, runtime).inserted &&
              pal4::DialogManagerEntryTree::Insert(state, 8, runtime).inserted,
          "standard interior nodes insert");
    CheckTree(state, runtime, {5, 8, 10, 11});

    result = pal4::DialogManagerEntryTree::Insert(state, 7, runtime);
    Check(result.inserted && result.node == FindNode(runtime, 7),
          "predecessor gap inserts node");
    CheckTree(state, runtime, {5, 7, 8, 10, 11});
}

void TestTreeHelpers() {
    TreeRuntime allocation;
    const auto allocated = pal4::DialogManagerEntryTree::CreateNode(
        TreeRuntime::kHead, 1, allocation);
    Check(allocation.NodeWord(allocated, 4) == TreeRuntime::kHead &&
              allocation.NodeWord(allocated, 16) == 1 &&
              allocation.NodeWord(allocated, 0) == 0 &&
              allocation.NodeWord(allocated, 8) == 0,
          "node helper writes only parent and color");

    pal4::DialogManagerState state;
    state.SetWord(4, TreeRuntime::kHead);
    TreeRuntime runtime;
    for (const auto key : {10u, 11u, 5u, 8u})
        Check(pal4::DialogManagerEntryTree::Insert(
                  state, key, runtime).inserted,
              "helper tree insertion");
    auto node = FindNode(runtime, 8);
    Check(pal4::DialogManagerEntryTree::Predecessor(node, runtime) ==
              FindNode(runtime, 5) && node == FindNode(runtime, 5),
          "predecessor climbs when left child is nil");
    node = FindNode(runtime, 10);
    Check(pal4::DialogManagerEntryTree::Predecessor(node, runtime) ==
              TreeRuntime::kNil && node == FindNode(runtime, 8),
          "predecessor takes rightmost node of left subtree");
    node = TreeRuntime::kHead;
    Check(pal4::DialogManagerEntryTree::Predecessor(node, runtime) ==
              FindNode(runtime, 11) && node == FindNode(runtime, 11),
          "header predecessor returns maximum");
    node = FindNode(runtime, 5);
    Check(pal4::DialogManagerEntryTree::Predecessor(node, runtime) ==
              TreeRuntime::kHead && node == TreeRuntime::kHead,
          "minimum predecessor returns header");

    pal4::DialogManagerState rotation_state;
    rotation_state.SetWord(4, TreeRuntime::kHead);
    TreeRuntime rotations;
    Check(pal4::DialogManagerEntryTree::Insert(rotation_state, 10,
                                                rotations).inserted &&
              pal4::DialogManagerEntryTree::Insert(rotation_state, 20,
                                                    rotations).inserted,
          "rotation fixture insertion");
    const auto first = FindNode(rotations, 10);
    const auto second = FindNode(rotations, 20);
    Check(pal4::DialogManagerEntryTree::RotateLeft(
              rotation_state, first, rotations) == second &&
              rotations.NodeWord(TreeRuntime::kHead, 4) == second &&
              rotations.NodeWord(second, 0) == first &&
              rotations.NodeWord(first, 4) == second,
          "left rotation returns pivot and rewires root");
    Check(pal4::DialogManagerEntryTree::RotateRight(
              rotation_state, second, rotations) == first &&
              rotations.NodeWord(TreeRuntime::kHead, 4) == first &&
              rotations.NodeWord(first, 8) == second &&
              rotations.NodeWord(second, 4) == first,
          "right rotation restores root and returns pivot");
    CheckTree(rotation_state, rotations, {10, 20});
}

void TestInsertAtOutput() {
    pal4::DialogManagerState state;
    state.SetWord(4, TreeRuntime::kHead);
    TreeRuntime runtime;
    std::uint32_t output = 0;
    Check(&pal4::DialogManagerEntryTree::InsertAt(
              state, output, TreeRuntime::kNil, TreeRuntime::kHead, 42,
              runtime) == &output && output == FindNode(runtime, 42),
          "direct insert returns output slot and allocated node");
    CheckTree(state, runtime, {42});
}

void TestErasePermutations() {
    std::vector<std::uint32_t> insertion{1, 2, 3, 4, 5};
    std::size_t leaf_erases = 0;
    std::size_t one_child_erases = 0;
    std::size_t two_child_erases = 0;
    do {
        std::vector<std::uint32_t> erasure{1, 2, 3, 4, 5};
        do {
            pal4::DialogManagerState state;
            state.SetWord(4, TreeRuntime::kHead);
            state.SetByte(8, 1);
            TreeRuntime runtime;
            std::vector<std::uint32_t> remaining{1, 2, 3, 4, 5};
            for (const auto key : insertion)
                Check(pal4::DialogManagerEntryTree::Insert(
                          state, key, runtime).inserted,
                      "permutation insertion");
            CheckTree(state, runtime, remaining);
            for (const auto key : erasure) {
                const auto node = FindNode(runtime, key);
                const auto left = runtime.NodeWord(node, 0);
                const auto right = runtime.NodeWord(node, 8);
                if (left != TreeRuntime::kNil && right != TreeRuntime::kNil)
                    ++two_child_erases;
                else if (left != TreeRuntime::kNil ||
                         right != TreeRuntime::kNil)
                    ++one_child_erases;
                else
                    ++leaf_erases;
                const auto following = std::upper_bound(
                    remaining.begin(), remaining.end(), key);
                const auto successor = following == remaining.end()
                                           ? TreeRuntime::kHead
                                           : FindNode(runtime, *following);
                std::uint32_t output = 0xffffffff;
                Check(&pal4::DialogManagerEntryTree::Erase(
                          state, output, node, runtime) == &output &&
                          output == successor,
                      "erase returns output iterator and original successor");
                remaining.erase(std::lower_bound(remaining.begin(),
                                                 remaining.end(), key));
                CheckTree(state, runtime, remaining);
            }
            Check(runtime.freed_nodes == insertion.size(),
                  "each inserted node freed once");
        } while (std::next_permutation(erasure.begin(), erasure.end()));
    } while (std::next_permutation(insertion.begin(), insertion.end()));
    Check(leaf_erases > 0 && one_child_erases > 0 && two_child_erases > 0,
          "erase exercises every node shape");
}

}  // namespace

int main() {
    try {
        TestExtendedTree();
        TestStandardTree();
        TestTreeHelpers();
        TestInsertAtOutput();
        TestErasePermutations();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
