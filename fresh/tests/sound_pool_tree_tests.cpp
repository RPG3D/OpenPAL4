#include "pal4/sound_pool_tree.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

class TreeRuntime final : public pal4::ISoundPoolTreeEraseRuntime {
public:
    static constexpr std::uint32_t pool = 0x2000u;
    static constexpr std::uint32_t head = 0x1000u;
    static constexpr std::uint32_t nil = 0x9000u;
    static constexpr std::uint32_t entry = 0x3000u;

    TreeRuntime() {
        WriteWord(pool, 4u, head);
        WriteWord(head, 0u, head);
        WriteWord(head, 4u, nil);
        WriteWord(head, 8u, head);
        WriteWord(nil, 24u, 1u);
    }

    std::uint32_t Sentinel() override { return nil; }
    std::uint32_t ReadWord(std::uint32_t address,
                           std::uint32_t offset) override {
        return words[address + offset];
    }
    void WriteWord(std::uint32_t address, std::uint32_t offset,
                   std::uint32_t value) override {
        const auto location = address + offset;
        words[location] = value;
        bytes.erase(location);
    }
    std::uint8_t ReadByte(std::uint32_t address,
                          std::uint32_t offset) override {
        const auto location = address + offset;
        if (const auto found = bytes.find(location); found != bytes.end())
            return found->second;
        return static_cast<std::uint8_t>(words[location]);
    }
    void WriteByte(std::uint32_t address, std::uint32_t offset,
                   std::uint8_t value) override {
        const auto location = address + offset;
        bytes[location] = value;
        words[location] = (words[location] & ~0xffu) | value;
    }
    std::uint32_t CreateNode(std::uint32_t parent) override {
        const auto node = next_node;
        next_node += 0x100u;
        WriteWord(node, 4u, parent);
        WriteWord(node, 24u, 0u);
        return node;
    }
    void CleanupEntry(std::uint32_t value_address) override {
        events.push_back("cleanup-entry");
        cleaned.push_back(value_address - 12u);
    }
    void FreeNode(std::uint32_t node) override {
        events.push_back("free-node");
        Check(freed.insert(node).second, "each erased node is freed once");
    }
    std::uint32_t Find(std::uint32_t key) {
        auto node = ReadWord(head, 4u);
        while (node != nil) {
            const auto current = ReadWord(node, 12u);
            if (key == current)
                return node;
            node = ReadWord(node, key < current ? 0u : 8u);
        }
        return head;
    }
    std::uint32_t VerifyNode(std::uint32_t node, std::uint32_t parent,
                             std::uint32_t lower, std::uint32_t upper,
                             std::set<std::uint32_t>& seen) {
        if (node == nil)
            return 1u;
        Check(seen.insert(node).second, "tree has no cycles");
        const auto key = ReadWord(node, 12u);
        Check(key > lower && key < upper && ReadWord(node, 4u) == parent,
              "tree keeps sorted keys and parent pointers");
        const auto left = ReadWord(node, 0u);
        const auto right = ReadWord(node, 8u);
        const auto color = ReadWord(node, 24u);
        Check(color <= 1u, "tree colors are red or black");
        if (color == 0u)
            Check(ReadWord(left, 24u) == 1u &&
                      ReadWord(right, 24u) == 1u,
                  "red node has black children");
        const auto left_height = VerifyNode(left, node, lower, key, seen);
        const auto right_height = VerifyNode(right, node, key, upper, seen);
        Check(left_height == right_height, "all paths have equal black height");
        return left_height + color;
    }
    void Verify(std::size_t expected_count) {
        std::set<std::uint32_t> seen;
        const auto root = ReadWord(head, 4u);
        Check(root == nil || ReadWord(root, 24u) == 1u,
              "root stays black");
        (void)VerifyNode(root, head, 0u, 1000u, seen);
        Check(seen.size() == expected_count &&
                  ReadWord(pool, 12u) == expected_count,
              "tree count matches reachable nodes");
        if (expected_count == 0u) {
            Check(ReadWord(head, 0u) == head &&
                      ReadWord(head, 8u) == head,
                  "empty tree resets both extrema");
        } else {
            Check(seen.count(ReadWord(head, 0u)) == 1u &&
                      seen.count(ReadWord(head, 8u)) == 1u,
                  "extrema point at live nodes");
            std::uint32_t smallest = 1000u;
            std::uint32_t largest = 0u;
            for (const auto node : seen) {
                const auto key = ReadWord(node, 12u);
                smallest = std::min(smallest, key);
                largest = std::max(largest, key);
            }
            Check(ReadWord(ReadWord(head, 0u), 12u) == smallest &&
                      ReadWord(ReadWord(head, 8u), 12u) == largest,
                  "head tracks actual minimum and maximum");
        }
    }
    pal4::SoundPoolInsertResult Insert(std::uint32_t key) {
        WriteWord(entry, 0u, key);
        WriteWord(entry, 4u, 0x55u);
        WriteWord(entry, 8u, 0x66u);
        return pal4::SoundPoolTree::Insert(pool, entry, *this);
    }

    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> events;
    std::uint32_t next_node = 0x4000u;
    std::vector<std::uint32_t> cleaned;
    std::set<std::uint32_t> freed;
};

void UniqueAndDuplicateBranches() {
    TreeRuntime tree;
    const auto first = tree.Insert(50u);
    Check(first.node == 0x4000u && first.inserted &&
              tree.ReadWord(TreeRuntime::head, 4u) == first.node &&
              tree.ReadWord(TreeRuntime::entry, 4u) == 0u &&
              tree.events.empty(),
          "empty unique tree creates its first node");
    tree.events.clear();
    const auto duplicate = tree.Insert(50u);
    Check(duplicate.node == 0x4000u &&
              !duplicate.inserted && tree.events.empty(),
          "existing CRC is returned without insertion");
    Check(tree.Insert(30u).inserted &&
              tree.ReadWord(TreeRuntime::head, 0u) == tree.Find(30u) &&
              tree.events.empty(),
          "left of minimum inserts directly");
    tree.events.clear();
    Check(tree.Insert(70u).inserted &&
              tree.ReadWord(TreeRuntime::head, 8u) == tree.Find(70u) &&
              tree.events.empty(),
          "right of maximum inserts after comparison");
    tree.events.clear();
    Check(tree.Insert(60u).inserted &&
              tree.Find(60u) != TreeRuntime::head && tree.events.empty(),
          "left search uses predecessor before unique insertion");
    tree.Verify(4u);
}

void InlineInsertions() {
    for (const auto& keys : {
             std::vector<std::uint32_t>{10u, 20u, 30u},
             std::vector<std::uint32_t>{30u, 20u, 10u},
             std::vector<std::uint32_t>{30u, 10u, 20u},
             std::vector<std::uint32_t>{10u, 30u, 20u},
             std::vector<std::uint32_t>{10u, 5u, 15u, 1u}}) {
        TreeRuntime tree;
        tree.bytes[TreeRuntime::pool + 8u] = 1u;
        for (const auto key : keys) {
            const auto result = tree.Insert(key);
            Check(result.inserted &&
                      tree.ReadWord(result.node, 12u) == key &&
                      tree.ReadWord(result.node, 16u) == 0x55u &&
                      tree.ReadWord(result.node, 20u) == 0x66u,
                  "inline branch allocates and copies full entry");
        }
        const auto root = tree.ReadWord(TreeRuntime::head, 4u);
        Check(tree.ReadWord(root, 12u) == 20u || keys.size() == 4u,
              "three-node rotations place median at root");
        Check(tree.ReadWord(root, 24u) == 1u &&
                  tree.ReadWord(TreeRuntime::pool, 12u) == keys.size() &&
                  tree.ReadWord(TreeRuntime::head, 0u) != TreeRuntime::head &&
                  tree.ReadWord(TreeRuntime::head, 8u) != TreeRuntime::head,
              "inline branch maintains count, black root and extrema");
        if (keys.size() == 4u)
            Check(tree.ReadWord(0x4100u, 24u) == 1u &&
                      tree.ReadWord(0x4200u, 24u) == 1u &&
                      tree.ReadWord(0x4300u, 24u) == 0u,
                  "red uncle recolors both parents and promotes grandparent");
    }
}

void EraseBranches() {
    const std::vector<std::uint32_t> keys{
        40u, 20u, 60u, 10u, 30u, 50u, 70u, 5u,
        15u, 25u, 35u, 45u, 55u, 65u, 75u};
    std::vector<std::vector<std::uint32_t>> orders{
        {5u, 15u, 25u, 35u, 45u, 55u, 65u, 75u,
         10u, 30u, 50u, 70u, 20u, 60u, 40u},
        {40u, 20u, 60u, 10u, 30u, 50u, 70u, 5u,
         15u, 25u, 35u, 45u, 55u, 65u, 75u},
        {75u, 70u, 65u, 60u, 55u, 50u, 45u, 40u,
         35u, 30u, 25u, 20u, 15u, 10u, 5u}};
    std::mt19937 generator(0x65ac70u);
    for (int sample = 0; sample != 16; ++sample) {
        auto shuffled = keys;
        std::shuffle(shuffled.begin(), shuffled.end(), generator);
        orders.push_back(std::move(shuffled));
    }
    for (const auto& order : orders) {
        TreeRuntime tree;
        tree.bytes[TreeRuntime::pool + 8u] = 1u;
        std::set<std::uint32_t> live(keys.begin(), keys.end());
        for (const auto key : keys)
            Check(tree.Insert(key).inserted, "setup inserts unique key");
        tree.Verify(live.size());
        for (const auto key : order) {
            const auto node = tree.Find(key);
            Check(node != TreeRuntime::head,
                  "erase target exists before removal");
            const auto next_key = live.upper_bound(key);
            const auto expected_next = next_key == live.end()
                ? TreeRuntime::head : tree.Find(*next_key);
            const auto cleaned_before = tree.cleaned.size();
            const auto result = pal4::SoundPoolTree::Erase(
                TreeRuntime::pool, node, tree);
            Check(result == expected_next && tree.freed.count(node) == 1u &&
                      tree.cleaned.size() == cleaned_before + 1u &&
                      tree.cleaned.back() == node,
                  "erase returns old successor and cleans original value");
            live.erase(key);
            tree.Verify(live.size());
            Check(tree.Find(key) == TreeRuntime::head,
                  "erased key is no longer searchable");
        }
    }
}

void EraseOwnershipGates() {
    for (const auto gate : {0u, 1u}) {
        TreeRuntime tree;
        tree.bytes[TreeRuntime::pool + 8u] = 1u;
        const auto node = tree.Insert(42u).node;
        tree.WriteWord(node, gate == 0u ? 16u : 20u, 0u);
        Check(pal4::SoundPoolTree::Erase(TreeRuntime::pool, node, tree) ==
                  TreeRuntime::head && tree.cleaned.empty() &&
                  tree.freed.count(node) == 1u,
              "unowned or null file skips value cleanup but frees node");
        tree.Verify(0u);
    }
}

void NavigationAndSubtreeCleanup() {
    TreeRuntime empty;
    Check(pal4::SoundPoolTree::FindExact(
              TreeRuntime::pool, 42u, empty) == TreeRuntime::head &&
              pal4::SoundPoolTree::LowerBound(
                  TreeRuntime::pool, 42u, empty) == TreeRuntime::head &&
              pal4::SoundPoolTree::DestroySubtree(
                  TreeRuntime::nil, empty) == TreeRuntime::nil &&
              empty.events.empty(),
          "empty search and subtree cleanup return tree sentinels");

    TreeRuntime tree;
    tree.bytes[TreeRuntime::pool + 8u] = 1u;
    for (const auto key : {20u, 10u, 30u})
        Check(tree.Insert(key).inserted, "navigation setup inserts key");
    Check(pal4::SoundPoolTree::FindExact(
              TreeRuntime::pool, 10u, tree) == tree.Find(10u) &&
              pal4::SoundPoolTree::FindExact(
                  TreeRuntime::pool, 20u, tree) == tree.Find(20u) &&
              pal4::SoundPoolTree::FindExact(
                  TreeRuntime::pool, 25u, tree) == TreeRuntime::head &&
              pal4::SoundPoolTree::LowerBound(
                  TreeRuntime::pool, 0u, tree) == tree.Find(10u) &&
              pal4::SoundPoolTree::LowerBound(
                  TreeRuntime::pool, 25u, tree) == tree.Find(30u) &&
              pal4::SoundPoolTree::LowerBound(
                  TreeRuntime::pool, 31u, tree) == TreeRuntime::head,
          "exact and lower-bound searches distinguish absent keys");
    Check(pal4::SoundPoolTree::Next(tree.Find(10u), tree) ==
                  tree.Find(20u) &&
              pal4::SoundPoolTree::Next(tree.Find(20u), tree) ==
                  tree.Find(30u) &&
              pal4::SoundPoolTree::Next(tree.Find(30u), tree) ==
                  TreeRuntime::head,
          "successor covers ancestor, right subtree and end iterator");
    Check(pal4::SoundPoolTree::Previous(tree.Find(10u), tree) ==
                  TreeRuntime::head &&
              pal4::SoundPoolTree::Previous(tree.Find(20u), tree) ==
                  tree.Find(10u) &&
              pal4::SoundPoolTree::Previous(tree.Find(30u), tree) ==
                  tree.Find(20u) &&
              pal4::SoundPoolTree::Previous(TreeRuntime::head, tree) ==
                  tree.Find(30u),
          "predecessor covers minimum, left subtree, ancestor and head");
    tree.events.clear();
    const auto root = tree.ReadWord(TreeRuntime::head, 4u);
    const auto left = tree.Find(10u);
    const auto right = tree.Find(30u);
    Check(pal4::SoundPoolTree::DestroySubtree(root, tree) ==
                  TreeRuntime::nil &&
              tree.cleaned == std::vector<std::uint32_t>{
                  right, root, left} &&
              tree.freed.size() == 3u &&
              tree.events == std::vector<std::string>{
                  "cleanup-entry", "free-node", "cleanup-entry",
                  "free-node", "cleanup-entry", "free-node"},
          "subtree cleanup recursively frees right side before left spine");
}

void RotationLinks() {
    TreeRuntime tree;
    tree.bytes[TreeRuntime::pool + 8u] = 1u;
    for (const auto key : {20u, 10u, 30u})
        Check(tree.Insert(key).inserted, "rotation setup inserts key");
    const auto root = tree.Find(20u);
    const auto right = tree.Find(30u);
    tree.WriteWord(right, 0u, 0x5000u);
    tree.WriteWord(0x5000u, 4u, right);
    tree.WriteWord(0x5000u, 0u, TreeRuntime::nil);
    tree.WriteWord(0x5000u, 8u, TreeRuntime::nil);
    Check(pal4::SoundPoolTree::RotateLeft(
              TreeRuntime::pool, root, tree) == right &&
              tree.ReadWord(TreeRuntime::head, 4u) == right &&
              tree.ReadWord(root, 8u) == 0x5000u &&
              tree.ReadWord(0x5000u, 4u) == root &&
              tree.ReadWord(right, 0u) == root &&
              tree.ReadWord(root, 4u) == right,
          "left rotation moves inner subtree and replaces root");
    Check(pal4::SoundPoolTree::RotateRight(
              TreeRuntime::pool, right, tree) == root &&
              tree.ReadWord(TreeRuntime::head, 4u) == root &&
              tree.ReadWord(right, 0u) == 0x5000u &&
              tree.ReadWord(0x5000u, 4u) == right,
          "right rotation restores root and inner subtree parent");
}

void EntryOwnershipTransfer() {
    TreeRuntime tree;
    tree.WriteWord(TreeRuntime::entry, 0u, 0x1234u);
    tree.WriteWord(TreeRuntime::entry, 4u, 0xaabbcc55u);
    tree.WriteWord(TreeRuntime::entry, 8u, 0x7000u);
    tree.WriteWord(0x5000u, 4u, 0x11223344u);
    Check(pal4::SoundPoolTree::CopyEntry(
              0x5000u, TreeRuntime::entry, tree) == 0x7000u &&
              tree.ReadWord(0x5000u, 0u) == 0x1234u &&
              tree.ReadWord(0x5000u, 4u) == 0x11223355u &&
              tree.ReadWord(0x5000u, 8u) == 0x7000u &&
              tree.ReadWord(TreeRuntime::entry, 4u) == 0xaabbcc00u,
          "entry copy moves only ownership byte and retains surrounding data");
    const auto original = tree.ReadWord(TreeRuntime::entry, 4u);
    Check(pal4::SoundPoolTree::CopyEntry(
              0u, TreeRuntime::entry, tree) == 0u &&
              tree.ReadWord(TreeRuntime::entry, 4u) == original,
          "null destination leaves source ownership untouched");
}

}  // namespace

int main() {
    UniqueAndDuplicateBranches();
    InlineInsertions();
    EraseBranches();
    EraseOwnershipGates();
    NavigationAndSubtreeCleanup();
    EntryOwnershipTransfer();
    RotationLinks();
    std::cout << "Sound pool tree tests passed\n";
}
