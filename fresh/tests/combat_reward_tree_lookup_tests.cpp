#include "pal4/combat_reward_tree_lookup.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
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

struct Runtime final : pal4::ICombatRewardTreeLookupRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::unordered_map<std::uint32_t, std::uint8_t> bytes;
    std::unordered_map<std::uint32_t, std::uint32_t> predecessors;
    std::vector<std::string> calls;
    std::uint32_t inserted = 0xa000u;
    std::uint32_t insert_calls = 0u;

    Runtime() {
        words[kTree + 4u] = kHead;
        words[kHead] = kHead;
        words[kHead + 4u] = kNil;
        words[kHead + 8u] = kHead;
    }
    std::uint32_t Nil() override { return kNil; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("word:" + std::to_string(address));
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.emplace_back("byte:" + std::to_string(address));
    }
    std::uint32_t Predecessor(std::uint32_t node) override {
        calls.emplace_back("predecessor:" + std::to_string(node));
        return predecessors.at(node);
    }
    std::uint32_t InsertAndRebalance(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t key) override {
        Check(tree == kTree && child == kNil && key == kKey,
              "insert receives tree, nil child and key pointer");
        ++insert_calls;
        calls.emplace_back("insert:" + std::to_string(parent));
        return inserted;
    }
};

void CheckResult(Runtime& runtime, std::uint32_t node, bool inserted) {
    Check(pal4::CombatRewardTreeLookup::FindOrInsert(
              kTree, kResult, kKey, runtime) == kResult &&
              runtime.words[kResult] == node &&
              runtime.bytes[kResult + 4u] == (inserted ? 1u : 0u) &&
              runtime.calls[runtime.calls.size() - 2u] == "word:16384" &&
              runtime.calls.back() == "byte:16388",
          "five-byte result stores node then inserted flag");
}

void EmptyAndRootCases() {
    Runtime empty;
    empty.words[kKey] = 42u;
    CheckResult(empty, empty.inserted, true);
    Check(empty.calls.front() == "insert:32768" &&
              empty.insert_calls == 1u,
          "empty tree inserts beneath head");

    Runtime equal;
    equal.words[kHead] = 0x8100u;
    equal.words[kHead + 4u] = 0x8100u;
    equal.words[0x8100u] = kNil;
    equal.words[0x8108u] = kNil;
    equal.words[0x810cu] = 42u;
    equal.words[kKey] = 42u;
    CheckResult(equal, 0x8100u, false);
    Check(equal.insert_calls == 0u,
          "existing equal key does not allocate node");

    Runtime lower = equal;
    lower.calls.clear();
    lower.words[kKey] = 10u;
    CheckResult(lower, lower.inserted, true);
    Check(lower.calls.front() == "insert:33024",
          "key below minimum inserts on left");

    Runtime higher = equal;
    higher.calls.clear();
    higher.words[kKey] = 50u;
    CheckResult(higher, higher.inserted, true);
    Check(higher.calls.front() == "insert:33024",
          "key above root inserts on right");
}

void PredecessorAndForcedInsert() {
    Runtime between;
    between.words[kHead] = 0x8200u;
    between.words[kHead + 4u] = 0x8100u;
    between.words[0x8100u] = 0x8200u;
    between.words[0x8108u] = 0x8300u;
    between.words[0x810cu] = 20u;
    between.words[0x8200u] = kNil;
    between.words[0x8208u] = kNil;
    between.words[0x820cu] = 10u;
    between.words[0x8300u] = kNil;
    between.words[0x8308u] = kNil;
    between.words[0x830cu] = 30u;
    between.predecessors[0x8300u] = 0x8100u;
    between.words[kKey] = 25u;
    CheckResult(between, between.inserted, true);
    Check(between.calls[0] == "predecessor:33536" &&
              between.calls[1] == "insert:33536",
          "left of nonminimum parent checks predecessor before insertion");

    Runtime forced = between;
    forced.calls.clear();
    forced.bytes[kTree + 8u] = 1u;
    forced.words[kKey] = 30u;
    CheckResult(forced, forced.inserted, true);
    Check(forced.calls.front() == "insert:33536",
          "tree byte flag forces insertion even for equal key");

    Runtime negative = between;
    negative.calls.clear();
    negative.words[kKey] = 0xffffffffu;
    CheckResult(negative, negative.inserted, true);
    Check(negative.calls.front() == "insert:33280",
          "key ordering is signed 32-bit");
}

void PairWritesInNativeOrder() {
    Runtime runtime;
    runtime.words[0x5000u] = 17u;
    runtime.words[0x6000u] = 29u;
    Check(pal4::CombatRewardTreeLookup::SetPair(
              0x7000u, 0x5000u, 0x6000u, runtime) == 0x7000u &&
              runtime.words[0x7000u] == 17u &&
              runtime.words[0x7004u] == 29u &&
              runtime.calls[0] == "word:28672" &&
              runtime.calls[1] == "word:28676",
          "pair constructor copies first then second word");

    runtime.calls.clear();
    runtime.words[0x7000u] = 43u;
    (void)pal4::CombatRewardTreeLookup::SetPair(
        0x7000u, 0x5000u, 0x7000u, runtime);
    Check(runtime.words[0x7000u] == 17u &&
              runtime.words[0x7004u] == 17u,
          "second source aliases the first destination word");
}

void AlternateLookupOrder() {
    Runtime empty;
    empty.words[kKey] = 42u;
    Check(pal4::CombatRewardTreeLookup::FindOrInsertAlternate(
              kTree, kResult, kKey, empty) == kResult &&
              empty.words[kResult] == empty.inserted &&
              empty.bytes[kResult + 4u] == 1u &&
              empty.calls[1] == "byte:16388" &&
              empty.calls[2] == "word:16384",
          "ordinary insertion writes flag before node pointer");

    Runtime existing;
    existing.words[kHead] = 0x8100u;
    existing.words[kHead + 4u] = 0x8100u;
    existing.words[0x8100u] = kNil;
    existing.words[0x8108u] = kNil;
    existing.words[0x810cu] = 42u;
    existing.words[kKey] = 42u;
    (void)pal4::CombatRewardTreeLookup::FindOrInsertAlternate(
        kTree, kResult, kKey, existing);
    Check(existing.insert_calls == 0u &&
              existing.words[kResult] == 0x8100u &&
              existing.bytes[kResult + 4u] == 0u &&
              existing.calls[0] == "byte:16388" &&
              existing.calls[1] == "word:16384",
          "existing key writes clear flag before node pointer");

    Runtime forced = existing;
    forced.calls.clear();
    forced.bytes[kTree + 8u] = 1u;
    (void)pal4::CombatRewardTreeLookup::FindOrInsertAlternate(
        kTree, kResult, kKey, forced);
    Check(forced.insert_calls == 1u &&
              forced.calls[1] == "word:16384" &&
              forced.calls[2] == "byte:16388",
          "inline forced insertion writes node before flag");
}

}  // namespace

int main() {
    try {
        EmptyAndRootCases();
        PredecessorAndForcedInsert();
        PairWritesInNativeOrder();
        AlternateLookupOrder();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
