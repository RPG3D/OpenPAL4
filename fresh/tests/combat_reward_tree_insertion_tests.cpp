#include "pal4/combat_reward_tree_insertion.h"
#include "pal4/combat_reward_tree_lookup.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <new>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kTree = 0x1000u;
constexpr std::uint32_t kHead = 0x8000u;
constexpr std::uint32_t kNil = 0x9000u;
constexpr std::uint32_t kKey = 0x3000u;
constexpr std::uint32_t kResult = 0x4000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatRewardTreeInsertionRuntime,
                       pal4::ICombatRewardTreeLookupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t allocations = 0u;
    bool fail_allocation = false;

    Runtime() {
        words[kTree + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
        words[kNil + 20u] = 1u;
    }
    std::uint32_t Nil() override { return kNil; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return static_cast<std::uint8_t>(words[address]);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == 24u, "insert allocates 24-byte node");
        if (fail_allocation) return 0u;
        return 0xa000u + allocations++ * 0x20u;
    }
    std::uint32_t Predecessor(std::uint32_t node) override {
        constexpr auto iterator = 0x5000u;
        words[iterator] = node;
        (void)pal4::CombatRewardTreeIterator::Predecessor(iterator, *this);
        return words[iterator];
    }
    std::uint32_t InsertAndRebalance(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key) override {
        constexpr auto scratch = 0x5004u;
        Check(pal4::CombatRewardTreeInsertion::InsertAndRebalance(
                  tree, scratch, child, parent, key, *this) == scratch,
              "insertion returns scratch result address");
        return words[scratch];
    }
};

std::uint32_t Insert(Runtime& runtime, std::int32_t value,
                     std::uint32_t count) {
    runtime.words[kKey] = static_cast<std::uint32_t>(value);
    runtime.words[kKey + 4u] = count;
    auto parent = kHead;
    auto node = runtime.words[kHead + 4u];
    while (node != kNil) {
        parent = node;
        node = runtime.words[node +
            (value < static_cast<std::int32_t>(runtime.words[node + 12u]) ?
             0u : 8u)];
    }
    Check(pal4::CombatRewardTreeInsertion::InsertAndRebalance(
              kTree, kResult, kNil, parent, kKey, runtime) == kResult,
          "insert returns result address");
    const auto inserted = runtime.words[kResult];
    Check(runtime.words[inserted + 12u] ==
              static_cast<std::uint32_t>(value) &&
              runtime.words[inserted + 16u] == count,
          "node stores both pair words");
    return inserted;
}

std::uint32_t Inspect(Runtime& runtime, std::uint32_t node,
                      std::uint32_t parent, std::vector<std::int32_t>& values,
                      std::uint32_t depth = 0u) {
    if (node == kNil) return 1u;
    Check(depth < 40u, "tree has no cycles");
    Check(runtime.words[node + 4u] == parent,
          "node parent points back to owner");
    const auto left = runtime.words[node];
    const auto right = runtime.words[node + 8u];
    const auto color = runtime.words[node + 20u];
    Check(color <= 1u, "node color is red or black");
    if (color == 0u)
        Check(runtime.words[left + 20u] == 1u &&
                  runtime.words[right + 20u] == 1u,
              "red node has black children");
    const auto left_height = Inspect(runtime, left, node, values, depth + 1u);
    values.push_back(static_cast<std::int32_t>(runtime.words[node + 12u]));
    const auto right_height = Inspect(runtime, right, node, values, depth + 1u);
    Check(left_height == right_height, "all leaf paths have equal black height");
    return left_height + color;
}

void CheckTree(Runtime& runtime, std::size_t expected_count) {
    const auto root = runtime.words[kHead + 4u];
    Check(runtime.words[root + 20u] == 1u,
          "root is black after rebalance");
    std::vector<std::int32_t> values;
    (void)Inspect(runtime, root, kHead, values);
    Check(values.size() == expected_count &&
              std::is_sorted(values.begin(), values.end()) &&
              runtime.words[kTree + 12u] == expected_count,
          "node count and signed in-order key sequence");
    auto minimum = root;
    while (runtime.words[minimum] != kNil)
        minimum = runtime.words[minimum];
    auto maximum = root;
    while (runtime.words[maximum + 8u] != kNil)
        maximum = runtime.words[maximum + 8u];
    Check(runtime.words[kHead] == minimum &&
              runtime.words[kHead + 8u] == maximum,
          "head extrema track inserted nodes");
}

void RebalanceSequences() {
    constexpr std::array<std::array<std::int32_t, 8>, 5> sequences{{
        {{30, 20, 10, 0, 0, 0, 0, 0}},
        {{10, 20, 30, 0, 0, 0, 0, 0}},
        {{30, 10, 20, 0, 0, 0, 0, 0}},
        {{10, 30, 20, 0, 0, 0, 0, 0}},
        {{10, 5, 15, 1, 7, 12, 18, -1}},
    }};
    for (std::size_t which = 0u; which < sequences.size(); ++which) {
        Runtime runtime;
        const auto length = which == 4u ? 8u : 3u;
        for (std::size_t index = 0u; index < length; ++index) {
            (void)Insert(runtime, sequences[which][index],
                         static_cast<std::uint32_t>(index + 1u));
            CheckTree(runtime, index + 1u);
        }
    }

    Runtime duplicates;
    (void)Insert(duplicates, 7, 1u);
    (void)Insert(duplicates, 7, 2u);
    CheckTree(duplicates, 2u);
}

void DirectRotations() {
    Runtime runtime;
    runtime.words[kHead + 4u] = 0xa000u;
    runtime.words[0xa000u] = kNil;
    runtime.words[0xa004u] = kHead;
    runtime.words[0xa008u] = 0xa020u;
    runtime.words[0xa020u] = 0xa040u;
    runtime.words[0xa024u] = 0xa000u;
    runtime.words[0xa028u] = kNil;
    runtime.words[0xa040u] = kNil;
    runtime.words[0xa044u] = 0xa020u;
    runtime.words[0xa048u] = kNil;
    Check(pal4::CombatRewardTreeInsertion::RotateLeft(
              kTree, 0xa000u, runtime) == 0xa020u &&
              runtime.words[kHead + 4u] == 0xa020u &&
              runtime.words[0xa008u] == 0xa040u &&
              runtime.words[0xa044u] == 0xa000u &&
              runtime.words[0xa020u] == 0xa000u,
          "left rotation updates root and middle subtree parent");
    Check(pal4::CombatRewardTreeInsertion::RotateRight(
              kTree, 0xa020u, runtime) == 0xa000u &&
              runtime.words[kHead + 4u] == 0xa000u &&
              runtime.words[0xa008u] == 0xa020u &&
              runtime.words[0xa040u] == kNil &&
              runtime.words[0xa044u] == 0xa020u,
          "right rotation restores original links");
}

void AllocationFailure() {
    Runtime runtime;
    runtime.fail_allocation = true;
    runtime.words[kKey] = 9u;
    bool threw = false;
    try {
        (void)pal4::CombatRewardTreeInsertion::InsertAndRebalance(
            kTree, kResult, kNil, kHead, kKey, runtime);
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    Check(threw && runtime.words[kHead + 4u] == kNil &&
              runtime.words[kTree + 12u] == 0u,
          "null allocation raises without changing tree");
}

void LookupAndInsertionTogether() {
    Runtime runtime;
    constexpr std::array<std::int32_t, 9> keys{
        30, 10, 40, 20, 35, -5, 50, 15, 25};
    for (std::size_t index = 0; index < keys.size(); ++index) {
        runtime.words[kKey] = static_cast<std::uint32_t>(keys[index]);
        runtime.words[kKey + 4u] = static_cast<std::uint32_t>(index + 1u);
        (void)pal4::CombatRewardTreeLookup::FindOrInsertAlternate(
            kTree, kResult, kKey, runtime);
        Check(runtime.words[kResult + 4u] == 1u &&
                  runtime.words[runtime.words[kResult] + 12u] ==
                      static_cast<std::uint32_t>(keys[index]),
              "lookup inserts a distinct key through the real tree");
        CheckTree(runtime, index + 1u);
    }

    runtime.words[kKey] = 20u;
    const auto allocation_count = runtime.allocations;
    (void)pal4::CombatRewardTreeLookup::FindOrInsertAlternate(
        kTree, kResult, kKey, runtime);
    Check(runtime.words[kResult + 4u] == 0u &&
              runtime.words[runtime.words[kResult] + 12u] == 20u &&
              runtime.allocations == allocation_count,
          "existing key returns its node without allocation");
    CheckTree(runtime, keys.size());

    runtime.words[kTree + 8u] = 1u;
    (void)pal4::CombatRewardTreeLookup::FindOrInsertAlternate(
        kTree, kResult, kKey, runtime);
    Check(runtime.words[kResult + 4u] == 1u &&
              runtime.allocations == allocation_count + 1u,
          "tree flag forces duplicate insertion");
    CheckTree(runtime, keys.size() + 1u);
}

void PredecessorBranches() {
    Runtime runtime;
    constexpr auto iterator = 0x5000u;
    runtime.words[kHead + 20u] = 0u;
    runtime.words[kHead + 4u] = 0xa000u;
    runtime.words[kHead + 8u] = 0xa040u;
    runtime.words[0xa004u] = kHead;
    runtime.words[iterator] = kHead;
    Check(pal4::CombatRewardTreeIterator::Predecessor(iterator, runtime) ==
              0xa040u && runtime.words[iterator] == 0xa040u,
          "header predecessor is the maximum node");

    runtime.words[0xa000u] = 0xa020u;
    runtime.words[0xa004u] = kHead;
    runtime.words[0xa014u] = 1u;
    runtime.words[0xa020u] = kNil;
    runtime.words[0xa024u] = 0xa000u;
    runtime.words[0xa028u] = 0xa040u;
    runtime.words[0xa040u] = kNil;
    runtime.words[0xa044u] = 0xa020u;
    runtime.words[0xa048u] = kNil;
    runtime.words[iterator] = 0xa000u;
    Check(pal4::CombatRewardTreeIterator::Predecessor(iterator, runtime) ==
              kNil && runtime.words[iterator] == 0xa040u,
          "left subtree predecessor is its rightmost node");

    runtime.words[0xa008u] = 0xa060u;
    runtime.words[0xa060u] = kNil;
    runtime.words[0xa064u] = 0xa000u;
    runtime.words[0xa068u] = kNil;
    runtime.words[iterator] = 0xa060u;
    Check(pal4::CombatRewardTreeIterator::Predecessor(iterator, runtime) ==
              0xa000u && runtime.words[iterator] == 0xa000u,
          "right child predecessor is its parent");
}

}  // namespace

int main() {
    try {
        RebalanceSequences();
        DirectRotations();
        AllocationFailure();
        LookupAndInsertionTogether();
        PredecessorBranches();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
