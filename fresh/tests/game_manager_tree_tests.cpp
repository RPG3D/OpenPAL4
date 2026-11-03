#include "pal4/game_manager_tree.h"
#include "pal4/game_manager_collection_walker.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <set>
#include <utility>
#include <vector>

namespace {

using pal4::GameManagerTreeInsert;
using pal4::GameManagerTreeMemoryStub;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

std::uint32_t Field(const GameManagerTreeMemoryStub& memory,
                    std::uint32_t node, std::uint32_t offset) {
    return memory.ReadDword(node + offset);
}

void EmptyTreeAndUniqueKeys() {
    GameManagerTreeMemoryStub memory;
    const auto first = GameManagerTreeInsert::Insert(
        memory.kTree, 10, 0x5010u, memory);
    Check(first.inserted && Field(memory, first.node, 12u) == 10u &&
              Field(memory, first.node, 16u) == 0x5010u,
          "first insertion copies both data words");
    Check(Field(memory, memory.kHeader, 0u) == first.node &&
              Field(memory, memory.kHeader, 4u) == first.node &&
              Field(memory, memory.kHeader, 8u) == first.node &&
              Field(memory, first.node, 4u) == memory.kHeader &&
              Field(memory, first.node, 20u) == 1u &&
              Field(memory, memory.kTree, 12u) == 1u,
          "empty-tree insertion links header, root, color and count");

    const auto duplicate = GameManagerTreeInsert::Insert(
        memory.kTree, 10, 0x9999u, memory);
    Check(!duplicate.inserted && duplicate.node == first.node &&
              Field(memory, first.node, 16u) == 0x5010u &&
              Field(memory, memory.kTree, 12u) == 1u,
          "unique-key duplicate returns the existing node without mutation");

    const auto left = GameManagerTreeInsert::Insert(memory.kTree, -3, 0x5003u, memory);
    const auto right = GameManagerTreeInsert::Insert(memory.kTree, 21, 0x5021u, memory);
    Check(Field(memory, memory.kHeader, 0u) == left.node &&
              Field(memory, memory.kHeader, 8u) == right.node &&
              Field(memory, memory.kTree, 12u) == 3u,
          "left and right insertions update header extrema and count");
    Check(GameManagerTreeInsert::Insert(memory.kTree, -3, 1u, memory).node == left.node &&
              !GameManagerTreeInsert::Insert(memory.kTree, 21, 1u, memory).inserted,
          "duplicates found through predecessor and right-descent paths");
}

void DuplicateModeAndSignedComparison() {
    GameManagerTreeMemoryStub memory(true);
    const auto zero = GameManagerTreeInsert::Insert(memory.kTree, 0, 1u, memory);
    const auto negative = GameManagerTreeInsert::Insert(memory.kTree, -1, 2u, memory);
    const auto repeated = GameManagerTreeInsert::Insert(memory.kTree, 0, 3u, memory);
    Check(zero.inserted && negative.inserted && repeated.inserted &&
              zero.node != repeated.node && Field(memory, memory.kTree, 12u) == 3u,
          "nonzero mode accepts repeated keys");
    Check(static_cast<std::int32_t>(Field(memory, Field(memory, memory.kHeader, 0u), 12u)) == -1,
          "signed comparison places negative keys at the left extreme");
    Check(Field(memory, repeated.node, 16u) == 3u,
          "duplicate nodes retain their own values");
}

int ValidateSubtree(const GameManagerTreeMemoryStub& memory,
                    std::uint32_t node, std::uint32_t parent,
                    std::vector<std::int32_t>& keys,
                    std::set<std::uint32_t>& visited) {
    if (node == GameManagerTreeInsert::kNilNode) return 1;
    Check(visited.insert(node).second, "tree has no cycles or repeated links");
    Check(Field(memory, node, 4u) == parent, "each node has the correct parent");
    const auto left = Field(memory, node, 0u);
    const auto right = Field(memory, node, 8u);
    const auto left_height = ValidateSubtree(memory, left, node, keys, visited);
    keys.push_back(static_cast<std::int32_t>(Field(memory, node, 12u)));
    const auto right_height = ValidateSubtree(memory, right, node, keys, visited);
    Check(left_height == right_height, "both branches have equal black height");
    const auto color = Field(memory, node, 20u);
    Check(color == 0u || color == 1u, "color is red or black");
    if (color == 0u) {
        Check(Field(memory, left, 20u) == 1u &&
                  Field(memory, right, 20u) == 1u,
              "red nodes have black children");
    }
    return left_height + static_cast<int>(color);
}

void CheckTree(const GameManagerTreeMemoryStub& memory,
               std::vector<std::int32_t> expected) {
    const auto root = Field(memory, memory.kHeader, 4u);
    if (expected.empty()) {
        Check(root == GameManagerTreeInsert::kNilNode &&
                  Field(memory, memory.kHeader, 0u) == memory.kHeader &&
                  Field(memory, memory.kHeader, 8u) == memory.kHeader &&
                  Field(memory, memory.kTree, 12u) == 0u,
              "empty tree restores all three header links and count");
        return;
    }
    Check(Field(memory, root, 20u) == 1u, "root stays black");
    std::vector<std::int32_t> actual;
    std::set<std::uint32_t> visited;
    ValidateSubtree(memory, root, memory.kHeader, actual, visited);
    std::sort(expected.begin(), expected.end());
    Check(actual == expected && visited.size() == expected.size(),
          "in-order traversal preserves all signed keys");
    auto smallest = root;
    while (Field(memory, smallest, 0u) != GameManagerTreeInsert::kNilNode)
        smallest = Field(memory, smallest, 0u);
    auto largest = root;
    while (Field(memory, largest, 8u) != GameManagerTreeInsert::kNilNode)
        largest = Field(memory, largest, 8u);
    Check(Field(memory, memory.kHeader, 0u) == smallest &&
              Field(memory, memory.kHeader, 8u) == largest &&
              Field(memory, memory.kTree, 12u) == expected.size(),
          "header extrema and count match the reachable nodes");
}

std::uint32_t FindNode(const GameManagerTreeMemoryStub& memory,
                       const std::int32_t key) {
    auto node = Field(memory, memory.kHeader, 4u);
    while (node != GameManagerTreeInsert::kNilNode) {
        const auto current = static_cast<std::int32_t>(Field(memory, node, 12u));
        if (current == key) return node;
        node = Field(memory, node, key < current ? 0u : 8u);
    }
    Check(false, "requested deletion key exists in tree");
    return 0u;
}

void RemoveSequence(const std::vector<std::int32_t>& insertion_order,
                    const std::vector<std::int32_t>& removal_order) {
    GameManagerTreeMemoryStub memory;
    std::vector<std::int32_t> remaining;
    for (const auto key : insertion_order) {
        (void)GameManagerTreeInsert::Insert(memory.kTree, key,
                                            static_cast<std::uint32_t>(key),
                                            memory);
        remaining.push_back(key);
    }
    CheckTree(memory, remaining);
    for (const auto key : removal_order) {
        const auto node = FindNode(memory, key);
        const auto next = pal4::GameManagerTreeCursor::Next(node, memory);
        // The independent expected key comes from sorted keys, not tree links.
        auto sorted = remaining;
        std::sort(sorted.begin(), sorted.end());
        const auto expected_upper = std::upper_bound(sorted.begin(), sorted.end(), key);
        if (expected_upper == sorted.end()) {
            Check(next == memory.kHeader, "rightmost cursor advances to header");
        } else {
            Check(static_cast<std::int32_t>(Field(memory, next, 12u)) ==
                      *expected_upper,
                  "successor cursor advances to the next signed key");
        }
        Check(pal4::GameManagerTreeRemove::Remove(memory.kTree, node, memory) == next,
              "erase returns the precomputed successor address");
        Check(memory.FreedNodes().back() == node &&
                  memory.FreedNodes().size() ==
                      insertion_order.size() - remaining.size() + 1u,
              "erase frees the original node exactly once");
        remaining.erase(std::find(remaining.begin(), remaining.end(), key));
        CheckTree(memory, remaining);
    }
}

void DeletionShapesAndFixup() {
    RemoveSequence({10}, {10});
    RemoveSequence({10, 5}, {10, 5});
    RemoveSequence({10, 15}, {10, 15});
    RemoveSequence({10, 5, 15}, {10, 5, 15});
    RemoveSequence({10, 5, 15, 3, 7, 12, 18},
                   {10, 3, 18, 5, 7, 12, 15});
    RemoveSequence({10, 5, 15, 3, 7, 12, 18},
                   {3, 7, 12, 18, 5, 15, 10});
    for (std::uint32_t seed = 0u; seed < 100u; ++seed) {
        std::vector<std::int32_t> insertion;
        for (int key = -6; key <= 6; ++key) insertion.push_back(key);
        std::mt19937 random(seed);
        std::shuffle(insertion.begin(), insertion.end(), random);
        auto removal = insertion;
        std::shuffle(removal.begin(), removal.end(), random);
        RemoveSequence(insertion, removal);
    }
}

void SuccessorReplacementAndDuplicateErase() {
    GameManagerTreeMemoryStub unique;
    for (const auto key : {10, 5, 15, 12, 18})
        (void)GameManagerTreeInsert::Insert(unique.kTree, key,
                                            static_cast<std::uint32_t>(key + 100),
                                            unique);
    const auto original_root = FindNode(unique, 10);
    const auto successor = FindNode(unique, 12);
    Check(pal4::GameManagerTreeRemove::Remove(unique.kTree, original_root,
                                               unique) == successor &&
              Field(unique, unique.kHeader, 4u) == successor &&
              Field(unique, successor, 16u) == 112u &&
              unique.FreedNodes().back() == original_root,
          "two-child erase moves successor node while keeping its payload");
    CheckTree(unique, {5, 12, 15, 18});

    GameManagerTreeMemoryStub duplicate(true);
    const auto first = GameManagerTreeInsert::Insert(duplicate.kTree, 2, 101u,
                                                     duplicate).node;
    const auto second = GameManagerTreeInsert::Insert(duplicate.kTree, 2, 202u,
                                                      duplicate).node;
    Check(pal4::GameManagerTreeRemove::Remove(duplicate.kTree, first,
                                               duplicate) == second &&
              Field(duplicate, second, 16u) == 202u,
          "duplicate-key erase returns the next distinct node address");
    CheckTree(duplicate, {2});
    Check(pal4::GameManagerTreeRemove::Remove(duplicate.kTree, second,
                                               duplicate) == duplicate.kHeader,
          "last duplicate advances to end header");
    CheckTree(duplicate, {});
}

void DirectInsertHelper() {
    GameManagerTreeMemoryStub memory;
    const auto root = GameManagerTreeInsert::InsertAt(
        memory.kTree, GameManagerTreeInsert::kNilNode, memory.kHeader,
        4, 104u, memory);
    const auto left = GameManagerTreeInsert::InsertAt(
        memory.kTree, root, root, 2, 102u, memory);
    const auto right = GameManagerTreeInsert::InsertAt(
        memory.kTree, GameManagerTreeInsert::kNilNode, root,
        6, 106u, memory);
    Check(Field(memory, root, 0u) == left &&
              Field(memory, root, 8u) == right &&
              Field(memory, left, 16u) == 102u &&
              Field(memory, right, 16u) == 106u,
          "later-addressed insert helper links both sides and copies data");
    CheckTree(memory, {2, 4, 6});
}

void DirectRotations() {
    GameManagerTreeMemoryStub memory;
    for (const auto key : {4, 2, 6, 1, 3, 5, 7})
        (void)GameManagerTreeInsert::Insert(memory.kTree, key, key, memory);
    const auto root = FindNode(memory, 4);
    const auto right = FindNode(memory, 6);
    Check(pal4::GameManagerTreeRotation::Left(memory.kTree, root, memory) ==
              right &&
              Field(memory, memory.kHeader, 4u) == right &&
              Field(memory, right, 0u) == root &&
              Field(memory, root, 4u) == right,
          "left rotation replaces root and repairs parent links");
    Check(pal4::GameManagerTreeRotation::Right(memory.kTree, right, memory) ==
              root,
          "right rotation restores original root");
    CheckTree(memory, {1, 2, 3, 4, 5, 6, 7});

    const auto left_parent = FindNode(memory, 2);
    const auto left_child = FindNode(memory, 3);
    Check(pal4::GameManagerTreeRotation::Left(memory.kTree, left_parent,
                                                memory) == left_child &&
              Field(memory, root, 0u) == left_child &&
              Field(memory, left_child, 4u) == root &&
              Field(memory, left_child, 0u) == left_parent,
          "non-root left rotation replaces a left-child link");
    Check(pal4::GameManagerTreeRotation::Right(memory.kTree, left_child,
                                                 memory) == left_parent,
          "non-root right rotation restores a left-child link");

    const auto right_parent = FindNode(memory, 6);
    const auto right_child = FindNode(memory, 5);
    Check(pal4::GameManagerTreeRotation::Right(memory.kTree, right_parent,
                                                 memory) == right_child &&
              Field(memory, root, 8u) == right_child &&
              Field(memory, right_child, 4u) == root &&
              Field(memory, right_child, 8u) == right_parent,
          "non-root right rotation replaces a right-child link");
    Check(pal4::GameManagerTreeRotation::Left(memory.kTree, right_child,
                                                memory) == right_parent,
          "non-root left rotation restores a right-child link");
    CheckTree(memory, {1, 2, 3, 4, 5, 6, 7});
}

void LookupAndPreviousCursor() {
    GameManagerTreeMemoryStub empty;
    Check(pal4::GameManagerTreeLookup::LowerBound(empty.kTree, 3, empty) ==
              empty.kHeader,
          "empty lower-bound search returns header");
    GameManagerTreeMemoryStub memory;
    for (const auto key : {4, 2, 6, 1, 3, 5, 7})
        (void)GameManagerTreeInsert::Insert(memory.kTree, key, key, memory);
    for (const auto& [key, expected] :
         std::vector<std::pair<std::int32_t, std::uint32_t>>{
             {-2, FindNode(memory, 1)}, {1, FindNode(memory, 1)},
             {4, FindNode(memory, 4)}, {5, FindNode(memory, 5)},
             {8, memory.kHeader}})
        Check(pal4::GameManagerTreeLookup::LowerBound(memory.kTree, key,
                                                       memory) == expected,
              "lower-bound search uses signed comparisons and header fallback");

    auto cursor = FindNode(memory, 4);
    Check(pal4::GameManagerTreeCursor::Previous(cursor, memory) ==
              GameManagerTreeInsert::kNilNode &&
              cursor == FindNode(memory, 3),
          "left-subtree predecessor updates cursor but returns nil EAX");
    cursor = FindNode(memory, 5);
    Check(pal4::GameManagerTreeCursor::Previous(cursor, memory) ==
              FindNode(memory, 4) && cursor == FindNode(memory, 4),
          "predecessor ascends through left-child links");
    cursor = memory.kHeader;
    Check(pal4::GameManagerTreeCursor::Previous(cursor, memory) ==
              FindNode(memory, 7) && cursor == FindNode(memory, 7),
          "header predecessor selects the rightmost node");
    cursor = FindNode(memory, 1);
    Check(pal4::GameManagerTreeCursor::Previous(cursor, memory) ==
              memory.kHeader && cursor == memory.kHeader,
          "leftmost predecessor reaches the header");
}

void RotationsAndRecoloring() {
    for (const auto& sequence : std::vector<std::vector<std::int32_t>>{
             {3, 2, 1}, {1, 2, 3}, {3, 1, 2}, {1, 3, 2},
             {10, 5, 15, 1, 7, 12, 18, 0, 2, 6, 8, 11, 13, 17, 19},
             {std::numeric_limits<std::int32_t>::min(), 0,
              std::numeric_limits<std::int32_t>::max(), -1, 1}}) {
        GameManagerTreeMemoryStub memory;
        std::vector<std::int32_t> inserted;
        for (const auto key : sequence) {
            Check(GameManagerTreeInsert::Insert(memory.kTree, key,
                                                static_cast<std::uint32_t>(key),
                                                memory).inserted,
                  "distinct keys insert successfully");
            inserted.push_back(key);
            CheckTree(memory, inserted);
        }
    }
}

class WalkerRuntime final : public pal4::IGameManagerCollectionRuntime {
public:
    explicit WalkerRuntime(const GameManagerTreeMemoryStub& memory) : memory_(memory) {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        return memory_.ReadDword(address);
    }
    void Invoke(std::uint32_t object, std::uint32_t slot_offset) override {
        Check(slot_offset == 0x28u, "walker uses observed virtual slot");
        objects.push_back(object);
    }
    std::vector<std::uint32_t> objects;
private:
    const GameManagerTreeMemoryStub& memory_;
};

void CollectionWalkerIntegration() {
    GameManagerTreeMemoryStub memory;
    for (const auto& [key, object] : std::vector<std::pair<int, std::uint32_t>>{
             {4, 0x5040u}, {1, 0x5010u}, {7, 0x5070u}, {5, 0x5050u}})
        (void)GameManagerTreeInsert::Insert(memory.kTree, key, object, memory);
    pal4::ScreenEffectDispatcherState state{};
    state.tree_sentinel = memory.kHeader;
    WalkerRuntime runtime(memory);
    Check(pal4::GameManagerCollectionWalker::NotifySlot40(state, runtime) ==
              memory.kHeader &&
              runtime.objects == std::vector<std::uint32_t>{
                  0x5010u, 0x5040u, 0x5050u, 0x5070u},
          "inserted manager objects are visited in key order");
}

}  // namespace

int main() {
    EmptyTreeAndUniqueKeys();
    DuplicateModeAndSignedComparison();
    RotationsAndRecoloring();
    CollectionWalkerIntegration();
    DeletionShapesAndFixup();
    SuccessorReplacementAndDuplicateErase();
    DirectInsertHelper();
    DirectRotations();
    LookupAndPreviousCursor();
    std::cout << "9 game manager tree test groups passed\n";
}
