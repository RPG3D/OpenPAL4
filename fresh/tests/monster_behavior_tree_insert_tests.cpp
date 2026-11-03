#include "pal4/monster_behavior_tree.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <random>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kNil = 0x7000u;
constexpr std::uint32_t kHeader = 0x8000u;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMonsterBehaviorTreeRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t next{0x9000u};
    std::uint32_t nil{kNil};
    std::uint32_t references{1u};
    std::uint32_t allocations{};
    std::vector<std::uint32_t> freed;

    Runtime() {
        words[kNil] = 0u;
        words[kNil + 4u] = 0u;
        words[kNil + 8u] = 0u;
        words[kNil + 20u] = 1u;
    }
    void InitializeRedBlackTree(pal4::MonsterBehaviorTreeState&,
                                std::uint32_t) override {}
    std::uint32_t ReadDword(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 0x18u, "node size is 24 bytes");
        const auto address = next;
        next += 0x20u;
        ++allocations;
        return address;
    }
    void DestroySubtree(std::uint32_t, std::uint32_t) override {}
    void GetFirstElement(std::uint32_t) override {}
    void RemoveNode(std::uint32_t, std::uint32_t) override {}
    std::uint32_t SharedNil() override { return nil; }
    void SetSharedNil(std::uint32_t address) override { nil = address; }
    std::uint32_t SharedReferenceCount() override { return references; }
    void SetSharedReferenceCount(std::uint32_t count) override {
        references = count;
    }
    std::uint32_t FreeMemory(std::uint32_t address) override {
        freed.push_back(address);
        return 0u;
    }

    std::uint32_t Field(std::uint32_t node, std::uint32_t offset) const {
        return words.at(node + offset);
    }
};

struct Tree {
    Runtime runtime;
    pal4::MonsterBehaviorTreeState state{};

    explicit Tree(bool duplicates = false) {
        state.words[2] = kHeader;
        state.words[3] = duplicates ? 1u : 0u;
        runtime.words[kHeader] = kHeader;
        runtime.words[kHeader + 4u] = kNil;
        runtime.words[kHeader + 8u] = kHeader;
        runtime.words[kHeader + 20u] = 0u;
    }
    std::uint32_t Field(std::uint32_t node, std::uint32_t offset) const {
        return runtime.Field(node, offset);
    }
};

using Entry = std::pair<std::int32_t, std::uint32_t>;

int ValidateNode(const Tree& tree, std::uint32_t node,
                 std::uint32_t parent, std::set<std::uint32_t>& seen,
                 std::vector<Entry>& entries) {
    if (node == kNil) return 1;
    Check(seen.insert(node).second, "node is reachable only once");
    Check(tree.Field(node, 4u) == parent, "parent pointer");
    const auto left = tree.Field(node, 0u);
    const auto right = tree.Field(node, 8u);
    const auto color = tree.Field(node, 20u);
    Check(color <= 1u, "red or black node");
    if (color == 0u)
        Check(tree.Field(left, 20u) == 1u &&
                  tree.Field(right, 20u) == 1u,
              "red node children are black");
    const auto left_height = ValidateNode(tree, left, node, seen, entries);
    entries.emplace_back(static_cast<std::int32_t>(tree.Field(node, 12u)),
                         tree.Field(node, 16u));
    const auto right_height = ValidateNode(tree, right, node, seen, entries);
    Check(left_height == right_height, "equal black height");
    return left_height + static_cast<int>(color);
}

void Validate(const Tree& tree, std::vector<Entry> expected) {
    const auto root = tree.Field(kHeader, 4u);
    if (expected.empty()) {
        Check(root == kNil && tree.Field(kHeader, 0u) == kHeader &&
                  tree.Field(kHeader, 8u) == kHeader &&
                  tree.state.words[4] == 0u, "empty tree header");
        return;
    }
    Check(tree.Field(root, 20u) == 1u, "black root");
    std::set<std::uint32_t> seen;
    std::vector<Entry> actual;
    ValidateNode(tree, root, kHeader, seen, actual);
    std::stable_sort(expected.begin(), expected.end(),
                     [](const Entry& a, const Entry& b) {
                         return a.first < b.first;
                     });
    Check(actual == expected && seen.size() == expected.size() &&
              tree.state.words[4] == expected.size(),
          "in-order classID, factory and count");
    auto smallest = root;
    while (tree.Field(smallest, 0u) != kNil)
        smallest = tree.Field(smallest, 0u);
    auto largest = root;
    while (tree.Field(largest, 8u) != kNil)
        largest = tree.Field(largest, 8u);
    Check(tree.Field(kHeader, 0u) == smallest &&
              tree.Field(kHeader, 8u) == largest,
          "header records both extrema");
}

void UniqueAndSignedKeys() {
    Tree tree;
    std::vector<Entry> expected;
    for (const auto [key, factory] : std::vector<Entry>{
             {10, 1010u}, {-5, 1005u}, {30, 1030u},
             {0, 1000u}, {-20, 1020u}, {15, 1015u}}) {
        const auto result = pal4::MonsterBehaviorTree::Insert(
            tree.state, key, factory, tree.runtime);
        Check(result.inserted &&
                  static_cast<std::int32_t>(tree.Field(result.node, 12u)) == key &&
                  tree.Field(result.node, 16u) == factory,
              "inserted node keeps signed classID and factory");
        expected.emplace_back(key, factory);
        Validate(tree, expected);
    }
    const auto allocations = tree.runtime.allocations;
    for (const auto key : {-20, -5, 0, 10, 15, 30}) {
        const auto result = pal4::MonsterBehaviorTree::Insert(
            tree.state, key, 0xdeadbeefu, tree.runtime);
        Check(!result.inserted &&
                  static_cast<std::int32_t>(tree.Field(result.node, 12u)) == key &&
                  tree.runtime.allocations == allocations,
              "unique mode returns existing node without allocation");
        Validate(tree, expected);
    }
}

void RotationsAndRecoloring() {
    for (const auto keys : {std::vector<int>{1, 2, 3},
                            std::vector<int>{3, 2, 1},
                            std::vector<int>{1, 3, 2},
                            std::vector<int>{3, 1, 2}}) {
        Tree tree;
        std::vector<Entry> expected;
        for (const auto key : keys) {
            const auto result = pal4::MonsterBehaviorTree::Insert(
                tree.state, key, static_cast<std::uint32_t>(key + 100),
                tree.runtime);
            Check(result.inserted, "rotation fixture inserts each key");
            expected.emplace_back(key, static_cast<std::uint32_t>(key + 100));
            Validate(tree, expected);
        }
        Check(tree.Field(tree.Field(kHeader, 4u), 12u) == 2u,
              "all four insertion shapes promote the middle key");
    }
    for (std::uint32_t seed = 0u; seed < 6u; ++seed) {
        Tree tree;
        std::vector<int> keys;
        for (int key = -24; key < 24; ++key) keys.push_back(key);
        std::mt19937 generator(seed);
        std::shuffle(keys.begin(), keys.end(), generator);
        std::vector<Entry> expected;
        for (const auto key : keys) {
            const auto factory = static_cast<std::uint32_t>(key + 1000);
            Check(pal4::MonsterBehaviorTree::Insert(
                      tree.state, key, factory, tree.runtime).inserted,
                  "randomized unique insertion");
            expected.emplace_back(key, factory);
            Validate(tree, expected);
        }
    }
}

void DuplicatesAndHelpers() {
    Tree tree(true);
    std::vector<Entry> expected;
    for (const auto entry : std::vector<Entry>{
             {4, 1u}, {4, 2u}, {2, 3u}, {4, 4u}, {6, 5u}}) {
        const auto result = pal4::MonsterBehaviorTree::Insert(
            tree.state, entry.first, entry.second, tree.runtime);
        Check(result.inserted, "duplicate mode allocates repeated classID");
        expected.push_back(entry);
        Validate(tree, expected);
    }

    auto cursor = kHeader;
    Check(pal4::MonsterBehaviorTree::AdvancePredecessor(
              cursor, tree.runtime) == tree.Field(kHeader, 8u) &&
              cursor == tree.Field(kHeader, 8u),
          "header cursor selects rightmost node");
    cursor = tree.Field(kHeader, 0u);
    Check(pal4::MonsterBehaviorTree::AdvancePredecessor(
              cursor, tree.runtime) == kHeader && cursor == kHeader,
          "leftmost cursor ascends to header");
    cursor = tree.Field(kHeader, 4u);
    if (tree.Field(cursor, 0u) != kNil) {
        Check(pal4::MonsterBehaviorTree::AdvancePredecessor(
                  cursor, tree.runtime) == kNil && cursor != kNil,
              "left-subtree descent returns nil register");
    }

    Runtime raw;
    raw.words[0xa000u] = 0x1234u;
    raw.words[0xa004u] = 0u;
    raw.words[0xa008u] = 0x5678u;
    raw.words[0xa014u] = 99u;
    raw.next = 0xa000u;
    Check(pal4::MonsterBehaviorTree::CreateNode(
              kHeader, 0u, raw) == 0xa000u &&
              raw.Field(0xa000u, 4u) == kHeader &&
              raw.Field(0xa000u, 20u) == 0u &&
              raw.Field(0xa000u, 0u) == 0x1234u &&
              raw.Field(0xa000u, 8u) == 0x5678u,
          "create-node helper writes only parent and color");

    Tree direct;
    std::uint32_t output{};
    auto& reference = pal4::MonsterBehaviorTree::InsertNodeAt(
        direct.state, kNil, kHeader, 7, 0x77u, output, direct.runtime);
    Check(&reference == &output && output == direct.Field(kHeader, 4u),
          "direct insertion returns caller output slot");
    Validate(direct, {{7, 0x77u}});
}

void DirectRotations() {
    Tree tree;
    const auto ten = pal4::MonsterBehaviorTree::Insert(
        tree.state, 10, 110u, tree.runtime).node;
    const auto twenty = pal4::MonsterBehaviorTree::Insert(
        tree.state, 20, 120u, tree.runtime).node;
    Check(pal4::MonsterBehaviorTree::RotateLeft(
              tree.state, ten, tree.runtime) == twenty &&
              tree.Field(kHeader, 4u) == twenty &&
              tree.Field(twenty, 0u) == ten &&
              tree.Field(ten, 4u) == twenty,
          "direct left rotation promotes right child and updates root");
    Check(pal4::MonsterBehaviorTree::RotateRight(
              tree.state, twenty, tree.runtime) == ten,
          "direct right rotation reverses promotion");
    Validate(tree, {{10, 110u}, {20, 120u}});
}

std::uint32_t FindNode(const Tree& tree, std::int32_t key) {
    auto node = tree.Field(kHeader, 4u);
    while (node != kNil) {
        const auto current = static_cast<std::int32_t>(tree.Field(node, 12u));
        if (key == current) return node;
        node = tree.Field(node, key < current ? 0u : 8u);
    }
    Check(false, "requested key is present");
    return 0u;
}

void InsertRemoveIntegration() {
    for (std::uint32_t seed = 0u; seed < 4u; ++seed) {
        Tree tree;
        std::vector<int> keys;
        for (int key = -16; key < 16; ++key) keys.push_back(key);
        std::mt19937 generator(seed + 100u);
        std::shuffle(keys.begin(), keys.end(), generator);
        std::vector<Entry> remaining;
        for (const auto key : keys) {
            const auto factory = static_cast<std::uint32_t>(key + 1000);
            (void)pal4::MonsterBehaviorTree::Insert(
                tree.state, key, factory, tree.runtime);
            remaining.emplace_back(key, factory);
        }
        Validate(tree, remaining);
        std::shuffle(keys.begin(), keys.end(), generator);
        for (const auto key : keys) {
            const auto node = FindNode(tree, key);
            std::int32_t next_key = 0;
            bool has_next = false;
            for (const auto& entry : remaining) {
                if (entry.first > key && (!has_next || entry.first < next_key)) {
                    next_key = entry.first;
                    has_next = true;
                }
            }
            const auto expected_successor = has_next
                ? FindNode(tree, next_key) : kHeader;
            std::uint32_t output{};
            auto& result = pal4::MonsterBehaviorTree::RemoveNode(
                tree.state, node, output, tree.runtime);
            Check(&result == &output && output == expected_successor &&
                      tree.runtime.freed.back() == node,
                  "inserted node removal returns successor and frees original");
            remaining.erase(std::find_if(remaining.begin(), remaining.end(),
                [key](const Entry& entry) { return entry.first == key; }));
            Validate(tree, remaining);
        }
    }
}

}  // namespace

int main() {
    UniqueAndSignedKeys();
    RotationsAndRecoloring();
    DuplicatesAndHelpers();
    DirectRotations();
    InsertRemoveIntegration();
}
