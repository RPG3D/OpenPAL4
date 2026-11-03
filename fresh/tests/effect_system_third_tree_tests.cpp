#include "pal4/effect_system.h"
#include "pal4/arena_sequence_resources.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <initializer_list>
#include <map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime : pal4::IEffectSystemThirdTreeInsertRuntime,
                 pal4::IArenaSequenceRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kNil = 0x93cdc8u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil}, {kHead + 8u, kHead},
        {kNil, kNil}, {kNil + 4u, 0u},
        {kNil + 8u, kNil}, {kNil + 20u, 1u}};
    bool allow_duplicates = false;
    std::uint32_t next_node = 0x4000u;
    std::vector<std::uint32_t> frees;

    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        Check(address == kTree + 8u,
              "insert reads third-tree duplicate mode");
        return allow_duplicates ? 1u : 0u;
    }
    std::uint8_t ReadByte(std::uint32_t) override {
        Check(false, "third-tree erasure never accesses text bytes");
        return 0u;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {
        Check(false, "third-tree erasure never changes text bytes");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return ReadDword(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        WriteDword(address, value);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t size) override {
        Check(size == 24u, "third-tree nodes are 24 bytes");
        const auto node = next_node;
        next_node += 0x40u;
        return node;
    }
    std::uint32_t Allocate(std::uint32_t) override {
        Check(false, "erasure allocates no new nodes");
        return 0u;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return 0u;
    }
    std::uint32_t Call(pal4::ArenaSequenceCall,
                       std::initializer_list<std::uint32_t>) override {
        Check(false, "third tree requires no external call");
        return 0u;
    }
    std::uint32_t Add(std::uint32_t key, std::uint32_t record) {
        words[record] = key;
        words[record + 4u] = key ^ 0x55aa55aau;
        const auto result = pal4::EffectSystemEmbeddedTrees::InsertThird(
            kTree, record, *this);
        Check(result.inserted &&
              ReadDword(result.node + 12u) == key &&
              ReadDword(result.node + 16u) == (key ^ 0x55aa55aau),
              "third-tree insertion copies both record words");
        return result.node;
    }
};

int Validate(const Runtime& runtime, std::uint32_t node,
             std::uint32_t parent, std::vector<std::uint32_t>& keys,
             std::uint32_t nil = Runtime::kNil) {
    if (node == nil) return 1;
    Check(runtime.ReadDword(node + 4u) == parent,
          "third-tree parent link");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 20u);
    Check(color <= 1u, "third-tree color is red or black");
    if (color == 0u)
        Check(runtime.ReadDword(left + 20u) == 1u &&
              runtime.ReadDword(right + 20u) == 1u,
              "red node has black children");
    const auto left_height = Validate(runtime, left, node, keys, nil);
    keys.push_back(runtime.ReadDword(node + 12u));
    const auto right_height = Validate(runtime, right, node, keys, nil);
    Check(left_height == right_height,
          "third-tree paths have equal black height");
    return left_height + static_cast<int>(color);
}

std::vector<std::uint32_t> OrderedKeys(
    const Runtime& runtime, std::uint32_t nil = Runtime::kNil) {
    std::vector<std::uint32_t> keys;
    const auto root = runtime.ReadDword(Runtime::kHead + 4u);
    if (root != nil)
        Check(runtime.ReadDword(root + 20u) == 1u,
              "third-tree root remains black");
    (void)Validate(runtime, root, Runtime::kHead, keys, nil);
    return keys;
}

struct FourthRuntime final : Runtime {
    static constexpr std::uint32_t kFourthNil = 0x93cdd0u;
    std::uint32_t SharedNil() const override { return kFourthNil; }
    FourthRuntime() {
        words[kHead + 4u] = kFourthNil;
        words[kFourthNil] = kFourthNil;
        words[kFourthNil + 4u] = 0u;
        words[kFourthNil + 8u] = kFourthNil;
        words[kFourthNil + 20u] = 1u;
    }
    std::uint32_t AddFourth(std::uint32_t key, std::uint32_t record) {
        words[record] = key;
        words[record + 4u] = key ^ 0x55aa55aau;
        const auto result = pal4::EffectSystemEmbeddedTrees::InsertFourth(
            kTree, record, *this);
        Check(result.inserted &&
              ReadDword(result.node + 12u) == key &&
              ReadDword(result.node + 16u) == (key ^ 0x55aa55aau),
              "fourth-tree insertion copies both record words");
        return result.node;
    }
};

struct FifthRuntime final : Runtime {
    static constexpr std::uint32_t kFifthNil = 0x93cdd8u;
    std::uint32_t SharedNil() const override { return kFifthNil; }
    FifthRuntime() {
        words[kHead + 4u] = kFifthNil;
        words[kFifthNil] = kFifthNil;
        words[kFifthNil + 4u] = 0u;
        words[kFifthNil + 8u] = kFifthNil;
        words[kFifthNil + 20u] = 1u;
    }
};

struct SecondRuntime final : Runtime {
    static constexpr std::uint32_t kSecondNil = 0x93cdc0u;
    std::uint32_t SharedNil() const override { return kSecondNil; }
    SecondRuntime() {
        words[kHead + 4u] = kSecondNil;
        words[kSecondNil] = kSecondNil;
        words[kSecondNil + 4u] = 0u;
        words[kSecondNil + 8u] = kSecondNil;
        words[kSecondNil + 20u] = 1u;
    }
};

void ReplaceNil(Runtime& runtime,
                const std::map<std::uint32_t, std::uint32_t>& nodes,
                std::uint32_t old_nil, std::uint32_t new_nil) {
    runtime.words[new_nil] = new_nil;
    runtime.words[new_nil + 4u] = 0u;
    runtime.words[new_nil + 8u] = new_nil;
    runtime.words[new_nil + 20u] = 1u;
    if (runtime.words.at(Runtime::kHead + 4u) == old_nil)
        runtime.words[Runtime::kHead + 4u] = new_nil;
    for (const auto& [key, node] : nodes) {
        (void)key;
        for (const auto offset : {0u, 8u}) {
            if (runtime.words.at(node + offset) == old_nil)
                runtime.words[node + offset] = new_nil;
        }
    }
}

void TestInsertLookupAndCursor() {
    Runtime runtime;
    const std::vector<std::uint32_t> input{
        0x80000000u, 1u, 0xffffffffu, 2u, 0x90000000u,
        3u, 0x70000000u, 4u};
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (std::size_t index = 0; index < input.size(); ++index)
        nodes[input[index]] = runtime.Add(
            input[index], 0x3000u + static_cast<std::uint32_t>(index) * 16u);
    Check(OrderedKeys(runtime) == (std::vector<std::uint32_t>{
              1u, 2u, 3u, 4u, 0x70000000u, 0x80000000u,
              0x90000000u, 0xffffffffu}) &&
          runtime.ReadDword(Runtime::kTree + 12u) == input.size(),
          "third-tree insertion orders unsigned keys and rebalances");
    for (const auto& [key, node] : nodes)
        Check(pal4::EffectSystemEmbeddedTrees::FindThird(
                  Runtime::kTree, key, Runtime::kNil, runtime) == node,
              "exact lookup finds each key");
    Check(pal4::EffectSystemEmbeddedTrees::FindThird(
              Runtime::kTree, 0u, Runtime::kNil, runtime) == Runtime::kHead &&
          pal4::EffectSystemEmbeddedTrees::FindThird(
              Runtime::kTree, 5u, Runtime::kNil, runtime) == Runtime::kHead &&
          pal4::EffectSystemEmbeddedTrees::FindThird(
              Runtime::kTree, 0xfffffffeu, Runtime::kNil, runtime) ==
              Runtime::kHead,
          "exact lookup returns the head below, between and above keys");

    auto iterator = runtime.ReadDword(Runtime::kHead);
    std::vector<std::uint32_t> traversed;
    while (iterator != Runtime::kHead) {
        traversed.push_back(runtime.ReadDword(iterator + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::AdvanceThird(
            iterator, Runtime::kNil, runtime);
        Check(iterator == step.next, "third cursor updates its iterator");
    }
    Check(traversed == OrderedKeys(runtime),
          "third-tree cursor traverses every key in order");

    runtime.words[0x3800u] = 0x80000000u;
    runtime.words[0x3804u] = 0u;
    const auto duplicate = pal4::EffectSystemEmbeddedTrees::InsertThird(
        Runtime::kTree, 0x3800u, runtime);
    Check(!duplicate.inserted && duplicate.node == nodes.at(0x80000000u) &&
          runtime.ReadDword(Runtime::kTree + 12u) == input.size(),
          "unique third tree rejects an equal unsigned key");
    runtime.allow_duplicates = true;
    const auto repeated = pal4::EffectSystemEmbeddedTrees::InsertThird(
        Runtime::kTree, 0x3800u, runtime);
    Check(repeated.inserted && repeated.node != duplicate.node &&
          runtime.ReadDword(Runtime::kTree + 12u) == input.size() + 1u,
          "multi third tree accepts equal keys");
}

void TestEraseAndRanges() {
    Runtime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (std::uint32_t key = 1u; key <= 15u; ++key)
        nodes[key] = runtime.Add(key, 0x3000u + key * 16u);
    for (const auto key : {8u, 1u, 15u, 6u, 4u, 12u}) {
        pal4::EffectSystemEmbeddedTrees::EraseThirdNode(
            Runtime::kTree, 0x5000u, nodes.at(key),
            Runtime::kNil, runtime);
        Check(runtime.frees.back() == nodes.at(key),
              "third-tree node erase frees its original node");
        nodes.erase(key);
        std::vector<std::uint32_t> expected;
        for (const auto& [remaining, pointer] : nodes) {
            (void)pointer;
            expected.push_back(remaining);
        }
        Check(OrderedKeys(runtime) == expected &&
              runtime.ReadDword(Runtime::kTree + 12u) == nodes.size() &&
              runtime.ReadDword(Runtime::kHead) == nodes.begin()->second &&
              runtime.ReadDword(Runtime::kHead + 8u) == nodes.rbegin()->second,
              "third-tree erasure keeps order, count and extrema");
    }

    pal4::EffectSystemEmbeddedTrees::EraseThirdRange(
        Runtime::kTree, 0x5000u, nodes.at(3u), nodes.at(9u),
        Runtime::kNil, runtime);
    for (const auto key : {3u, 5u, 7u}) nodes.erase(key);
    std::vector<std::uint32_t> expected;
    for (const auto& [key, pointer] : nodes) {
        (void)pointer;
        expected.push_back(key);
    }
    Check(OrderedKeys(runtime) == expected &&
          runtime.ReadDword(0x5000u) == nodes.at(9u),
          "third-tree partial range uses exclusive end");

    pal4::EffectSystemEmbeddedTrees::EraseThirdRange(
        Runtime::kTree, 0x5000u,
        runtime.ReadDword(Runtime::kHead), Runtime::kHead,
        Runtime::kNil, runtime);
    Check(runtime.ReadDword(0x5000u) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kTree + 12u) == 0u &&
          runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) == Runtime::kNil &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead &&
          runtime.frees.size() == 15u,
          "third-tree full range releases all remaining nodes");
}

struct ConstructRuntime final :
    pal4::IEffectSystemEmbeddedTreeConstructRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> order;
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        order.push_back(address);
    }
    void InitializeTree(std::uint32_t tree) override {
        Check(bytes.at(tree) == 0xa1u &&
              bytes.at(tree + 1u) == 0xb2u &&
              bytes.at(tree + 8u) == 1u,
              "third-tree helper sees all three flags");
        order.push_back(0xffffffffu);
    }
};

void TestConstructor() {
    ConstructRuntime runtime;
    Check(pal4::EffectSystemEmbeddedTrees::InitializeThird(
              Runtime::kTree, 0xa1u, 0xb2u, 1u, runtime) ==
              Runtime::kTree &&
          runtime.order == (std::vector<std::uint32_t>{
              Runtime::kTree, Runtime::kTree + 8u,
              Runtime::kTree + 1u, 0xffffffffu}),
          "third-tree constructor preserves flag-copy order");
}

void TestFourthInsertAndClear() {
    FourthRuntime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (const auto key : {8u, 4u, 12u, 2u, 6u, 10u, 14u})
        nodes[key] = runtime.AddFourth(key, 0x3000u + key * 16u);
    Check(OrderedKeys(runtime, FourthRuntime::kFourthNil) ==
              (std::vector<std::uint32_t>{2u, 4u, 6u, 8u,
                                          10u, 12u, 14u}) &&
          runtime.ReadDword(Runtime::kTree + 12u) == 7u,
          "fourth-tree insertion balances seven ordered keys");

    auto iterator = runtime.ReadDword(Runtime::kHead);
    std::vector<std::uint32_t> seen;
    while (iterator != Runtime::kHead) {
        seen.push_back(runtime.ReadDword(iterator + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::AdvanceFourth(
            iterator, FourthRuntime::kFourthNil, runtime);
        Check(step.next == iterator, "fourth successor updates iterator");
    }
    Check(seen == OrderedKeys(runtime, FourthRuntime::kFourthNil),
          "fourth-tree cursor visits every node");

    Check(pal4::EffectSystemEmbeddedTrees::ClearFourth(
              Runtime::kTree, 0x5000u,
              FourthRuntime::kFourthNil, runtime) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) ==
              FourthRuntime::kFourthNil &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kTree + 12u) == 0u &&
          runtime.frees.size() == nodes.size(),
          "fourth-tree clear releases nodes while retaining head and nil");
    const auto frees = runtime.frees.size();
    Check(pal4::EffectSystemEmbeddedTrees::ClearFourth(
              Runtime::kTree, 0x5000u,
              FourthRuntime::kFourthNil, runtime) == Runtime::kHead &&
          runtime.frees.size() == frees,
          "empty fourth-tree clear performs no further frees");

    const auto replacement = runtime.AddFourth(3u, 0x3800u);
    Check(replacement != 0u &&
          OrderedKeys(runtime, FourthRuntime::kFourthNil) ==
              (std::vector<std::uint32_t>{3u}),
          "fourth-tree head accepts insertion after clear");
}

void TestFifthEraseAndCursor() {
    FourthRuntime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (const auto key : {8u, 4u, 12u, 2u, 6u, 10u, 14u})
        nodes[key] = runtime.AddFourth(key, 0x3000u + key * 16u);
    constexpr std::uint32_t fifth_nil = 0x93cdd8u;
    ReplaceNil(runtime, nodes, FourthRuntime::kFourthNil, fifth_nil);
    auto iterator = runtime.ReadDword(Runtime::kHead);
    std::vector<std::uint32_t> seen;
    while (iterator != Runtime::kHead) {
        seen.push_back(runtime.ReadDword(iterator + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::AdvanceFifth(
            iterator, fifth_nil, runtime);
        Check(step.next == iterator, "fifth successor updates iterator");
    }
    Check(seen == OrderedKeys(runtime, fifth_nil),
          "fifth-tree cursor preserves sorted order");

    for (const auto key : {8u, 2u, 14u, 6u, 4u, 12u, 10u}) {
        pal4::EffectSystemEmbeddedTrees::EraseFifthNode(
            Runtime::kTree, 0x5000u, nodes.at(key),
            fifth_nil, runtime);
        Check(runtime.frees.back() == nodes.at(key),
              "fifth-tree erasure frees selected node");
        nodes.erase(key);
        std::vector<std::uint32_t> expected;
        for (const auto& [remaining, pointer] : nodes) {
            (void)pointer;
            expected.push_back(remaining);
        }
        Check(OrderedKeys(runtime, fifth_nil) == expected &&
              runtime.ReadDword(Runtime::kTree + 12u) == nodes.size(),
              "fifth-tree erasure preserves count and black heights");
    }
}

void TestFifthConstructor() {
    ConstructRuntime runtime;
    Check(pal4::EffectSystemEmbeddedTrees::InitializeFifth(
              Runtime::kTree, 0xa1u, 0xb2u, 1u, runtime) ==
              Runtime::kTree &&
          runtime.order == (std::vector<std::uint32_t>{
              Runtime::kTree, Runtime::kTree + 8u,
              Runtime::kTree + 1u, 0xffffffffu}),
          "fifth-tree constructor preserves flag-copy order");
}

void TestThirdPositionedInsertAndLinks() {
    Runtime inserted;
    inserted.words[0x3000u] = 0x80000000u;
    inserted.words[0x3004u] = 0x12345678u;
    const auto node = pal4::EffectSystemEmbeddedTrees::InsertThirdAt(
        Runtime::kTree, Runtime::kNil, Runtime::kHead,
        0x3000u, inserted);
    Check(node == 0x4000u &&
          inserted.ReadDword(node) == Runtime::kNil &&
          inserted.ReadDword(node + 4u) == Runtime::kHead &&
          inserted.ReadDword(node + 8u) == Runtime::kNil &&
          inserted.ReadDword(node + 12u) == 0x80000000u &&
          inserted.ReadDword(node + 16u) == 0x12345678u &&
          inserted.ReadDword(node + 20u) == 1u &&
          inserted.ReadDword(Runtime::kHead) == node &&
          inserted.ReadDword(Runtime::kHead + 4u) == node &&
          inserted.ReadDword(Runtime::kHead + 8u) == node &&
          inserted.ReadDword(Runtime::kTree + 12u) == 1u,
          "third-tree positioned insertion copies record and links root");

    Runtime runtime;
    constexpr auto root = 0x5000u;
    constexpr auto right = 0x5100u;
    constexpr auto moved = 0x5200u;
    runtime.words[Runtime::kHead + 4u] = root;
    runtime.words[root] = Runtime::kNil;
    runtime.words[root + 4u] = Runtime::kHead;
    runtime.words[root + 8u] = right;
    runtime.words[right] = moved;
    runtime.words[right + 4u] = root;
    runtime.words[right + 8u] = Runtime::kNil;
    runtime.words[moved] = Runtime::kNil;
    runtime.words[moved + 4u] = right;
    runtime.words[moved + 8u] = Runtime::kNil;
    Check(pal4::EffectSystemEmbeddedTrees::ThirdMinimumFromLink(
              root + 8u, Runtime::kNil, runtime) == moved &&
          pal4::EffectSystemEmbeddedTrees::ThirdMinimumFromLink(
              root, Runtime::kNil, runtime) == root,
          "third-tree leftmost lookup honors supplied child link");
    Check(pal4::EffectSystemEmbeddedTrees::PromoteThirdRightChild(
              Runtime::kTree, root, runtime) == right &&
          runtime.ReadDword(Runtime::kHead + 4u) == right &&
          runtime.ReadDword(right) == root &&
          runtime.ReadDword(root + 8u) == moved &&
          runtime.ReadDword(moved + 4u) == root,
          "third-tree right promotion relinks moved subtree");
    Check(pal4::EffectSystemEmbeddedTrees::PromoteThirdLeftChild(
              Runtime::kTree, right, runtime) == root &&
          runtime.ReadDword(Runtime::kHead + 4u) == root &&
          runtime.ReadDword(root + 8u) == right &&
          runtime.ReadDword(right) == moved &&
          runtime.ReadDword(moved + 4u) == right,
          "third-tree left promotion restores root links");
}

void TestFourthNodeErasure() {
    FourthRuntime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (const auto key : {8u, 4u, 12u, 2u, 6u, 10u, 14u})
        nodes[key] = runtime.AddFourth(key, 0x3000u + key * 16u);
    for (const auto key : {8u, 2u, 14u, 6u, 4u, 12u, 10u}) {
        pal4::EffectSystemEmbeddedTrees::EraseFourthNode(
            Runtime::kTree, 0x5000u, nodes.at(key),
            FourthRuntime::kFourthNil, runtime);
        Check(runtime.frees.back() == nodes.at(key),
              "fourth-tree erasure frees the selected node");
        nodes.erase(key);
        std::vector<std::uint32_t> expected;
        for (const auto& [remaining, pointer] : nodes) {
            (void)pointer;
            expected.push_back(remaining);
        }
        Check(OrderedKeys(runtime, FourthRuntime::kFourthNil) == expected &&
              runtime.ReadDword(Runtime::kTree + 12u) == nodes.size(),
              "fourth-tree erasure preserves order and black heights");
    }
    Check(runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) ==
              FourthRuntime::kFourthNil &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead,
          "fourth-tree head returns to empty links");
}

void TestFourthPositionedInsert() {
    FourthRuntime runtime;
    runtime.words[0x3000u] = 0x80000000u;
    runtime.words[0x3004u] = 0x12345678u;
    const auto node = pal4::EffectSystemEmbeddedTrees::InsertFourthAt(
        Runtime::kTree, FourthRuntime::kFourthNil,
        Runtime::kHead, 0x3000u, runtime);
    Check(node == 0x4000u &&
          runtime.ReadDword(node) == FourthRuntime::kFourthNil &&
          runtime.ReadDword(node + 4u) == Runtime::kHead &&
          runtime.ReadDword(node + 8u) == FourthRuntime::kFourthNil &&
          runtime.ReadDword(node + 12u) == 0x80000000u &&
          runtime.ReadDword(node + 16u) == 0x12345678u &&
          runtime.ReadDword(node + 20u) == 1u &&
          runtime.ReadDword(Runtime::kHead) == node &&
          runtime.ReadDword(Runtime::kHead + 4u) == node &&
          runtime.ReadDword(Runtime::kHead + 8u) == node &&
          runtime.ReadDword(Runtime::kTree + 12u) == 1u,
          "fourth-tree positioned insertion uses its own nil and head");
}

void TestFifthPositionedInsert() {
    FifthRuntime runtime;
    runtime.words[0x3000u] = 0x80000000u;
    runtime.words[0x3004u] = 0x12345678u;
    const auto node = pal4::EffectSystemEmbeddedTrees::InsertFifthAt(
        Runtime::kTree, FifthRuntime::kFifthNil,
        Runtime::kHead, 0x3000u, runtime);
    Check(node == 0x4000u &&
          runtime.ReadDword(node) == FifthRuntime::kFifthNil &&
          runtime.ReadDword(node + 4u) == Runtime::kHead &&
          runtime.ReadDword(node + 8u) == FifthRuntime::kFifthNil &&
          runtime.ReadDword(node + 12u) == 0x80000000u &&
          runtime.ReadDword(node + 16u) == 0x12345678u &&
          runtime.ReadDword(node + 20u) == 1u &&
          runtime.ReadDword(Runtime::kHead) == node &&
          runtime.ReadDword(Runtime::kHead + 4u) == node &&
          runtime.ReadDword(Runtime::kHead + 8u) == node &&
          runtime.ReadDword(Runtime::kTree + 12u) == 1u,
          "fifth-tree positioned insertion uses its own nil and head");
}

void TestLaterTreePredecessors() {
    using Previous = pal4::MonsterManagerTreeStep (*)(
        std::uint32_t&, std::uint32_t,
        pal4::IArenaSequenceRuntime&);
    const auto check_reverse = [](Runtime& runtime, std::uint32_t nil,
                                  Previous previous) {
        auto iterator = runtime.ReadDword(Runtime::kHead + 8u);
        std::vector<std::uint32_t> seen;
        while (iterator != Runtime::kHead) {
            seen.push_back(runtime.ReadDword(iterator + 12u));
            const auto step = previous(iterator, nil, runtime);
            Check(step.next == iterator,
                  "later-tree predecessor updates its iterator");
        }
        Check(seen == (std::vector<std::uint32_t>{7u, 6u, 5u, 4u,
                                                  3u, 2u, 1u}),
              "later-tree predecessor traverses reverse order");
    };

    Runtime third;
    FourthRuntime fourth;
    FifthRuntime fifth;
    for (const auto key : {4u, 2u, 6u, 1u, 3u, 5u, 7u}) {
        (void)third.Add(key, 0x3000u + key * 16u);
        (void)fourth.AddFourth(key, 0x3000u + key * 16u);
        (void)fifth.Add(key, 0x3000u + key * 16u);
    }
    check_reverse(third, Runtime::kNil,
                  pal4::EffectSystemEmbeddedTrees::PreviousThird);
    check_reverse(fourth, FourthRuntime::kFourthNil,
                  pal4::EffectSystemEmbeddedTrees::PreviousFourth);
    check_reverse(fifth, FifthRuntime::kFifthNil,
                  pal4::EffectSystemEmbeddedTrees::PreviousFifth);
}

void TestFourthRotations() {
    FourthRuntime runtime;
    constexpr auto root = 0x5000u;
    constexpr auto right = 0x5100u;
    constexpr auto moved = 0x5200u;
    runtime.words[Runtime::kHead + 4u] = root;
    runtime.words[root] = FourthRuntime::kFourthNil;
    runtime.words[root + 4u] = Runtime::kHead;
    runtime.words[root + 8u] = right;
    runtime.words[right] = moved;
    runtime.words[right + 4u] = root;
    runtime.words[right + 8u] = FourthRuntime::kFourthNil;
    runtime.words[moved] = FourthRuntime::kFourthNil;
    runtime.words[moved + 4u] = right;
    runtime.words[moved + 8u] = FourthRuntime::kFourthNil;
    Check(pal4::EffectSystemEmbeddedTrees::PromoteFourthRightChild(
              Runtime::kTree, root, runtime) == right &&
          runtime.ReadDword(Runtime::kHead + 4u) == right &&
          runtime.ReadDword(right) == root &&
          runtime.ReadDword(root + 8u) == moved &&
          runtime.ReadDword(moved + 4u) == root,
          "fourth-tree right promotion relinks root and moved child");
    Check(pal4::EffectSystemEmbeddedTrees::PromoteFourthLeftChild(
              Runtime::kTree, right, runtime) == root &&
          runtime.ReadDword(Runtime::kHead + 4u) == root &&
          runtime.ReadDword(root + 8u) == right &&
          runtime.ReadDword(right) == moved &&
          runtime.ReadDword(moved + 4u) == right,
          "fourth-tree left promotion restores original links");
}

void TestFirstExactAndLowerBoundEntries() {
    Runtime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    constexpr auto first_nil = 0x93cdb8u;
    Check(pal4::EffectSystemEmbeddedTrees::FindFirstExact(
              Runtime::kTree, 1u, Runtime::kNil, runtime) == Runtime::kHead &&
          pal4::EffectSystemEmbeddedTrees::LowerBoundFirst(
              Runtime::kTree, 1u, Runtime::kNil, runtime) == Runtime::kHead,
          "empty first tree returns its head");
    const std::vector<std::uint32_t> keys{
        4u, 2u, 6u, 1u, 3u, 5u, 7u, 0x80000000u, 0xffffffffu};
    for (std::size_t index = 0; index < keys.size(); ++index)
        nodes[keys[index]] = runtime.Add(
            keys[index], 0x3000u + static_cast<std::uint32_t>(index) * 16u);
    ReplaceNil(runtime, nodes, Runtime::kNil, first_nil);
    for (const auto& [key, node] : nodes) {
        std::uint32_t result = 0u;
        pal4::EffectSystemEmbeddedTrees::LowerBoundFirstInto(
            Runtime::kTree, result, key, first_nil, runtime);
        Check(result == node &&
              pal4::EffectSystemEmbeddedTrees::LowerBoundFirst(
                  Runtime::kTree, key, first_nil, runtime) == node &&
              pal4::EffectSystemEmbeddedTrees::FindFirstExact(
                  Runtime::kTree, key, first_nil, runtime) == node,
              "both lower-bound ABIs and exact lookup find each key");
    }
    std::uint32_t result = 0u;
    pal4::EffectSystemEmbeddedTrees::LowerBoundFirstInto(
        Runtime::kTree, result, 0u, first_nil, runtime);
    Check(result == nodes.at(1u) &&
          pal4::EffectSystemEmbeddedTrees::FindFirstExact(
              Runtime::kTree, 0u, first_nil, runtime) == Runtime::kHead,
          "below-minimum lookup distinguishes lower bound from exact match");
    Check(pal4::EffectSystemEmbeddedTrees::LowerBoundFirst(
              Runtime::kTree, 8u, first_nil, runtime) ==
              nodes.at(0x80000000u) &&
          pal4::EffectSystemEmbeddedTrees::LowerBoundFirst(
              Runtime::kTree, 0xffffffffu, first_nil, runtime) ==
              nodes.at(0xffffffffu) &&
          pal4::EffectSystemEmbeddedTrees::LowerBoundFirst(
              Runtime::kTree, 0x80000001u, first_nil, runtime) ==
              nodes.at(0xffffffffu) &&
          pal4::EffectSystemEmbeddedTrees::FindFirstExact(
              Runtime::kTree, 8u, first_nil, runtime) == Runtime::kHead,
          "first lookup compares high-bit keys as unsigned values");
}

void TestSecondPositionedInsertAndCursors(
    const std::vector<std::uint32_t>& input) {
    SecondRuntime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (const auto key : input) {
        const auto record = 0x3000u + key * 16u;
        runtime.words[record] = key;
        runtime.words[record + 4u] = key ^ 0x55aa55aau;
        auto parent = SecondRuntime::kHead;
        auto child = runtime.ReadDword(parent + 4u);
        while (child != SecondRuntime::kSecondNil) {
            parent = child;
            child = runtime.ReadDword(
                child + (key < runtime.ReadDword(child + 12u) ? 0u : 8u));
        }
        const auto node = pal4::EffectSystemEmbeddedTrees::InsertSecondAt(
            SecondRuntime::kTree, child, parent, record, runtime);
        nodes[key] = node;
        Check(runtime.ReadDword(node + 12u) == key &&
              runtime.ReadDword(node + 16u) == (key ^ 0x55aa55aau),
              "second positioned insert copies two record words");
    }
    std::vector<std::uint32_t> ordered;
    const auto root = runtime.ReadDword(SecondRuntime::kHead + 4u);
    (void)Validate(runtime, root, SecondRuntime::kHead, ordered,
                   SecondRuntime::kSecondNil);
    Check(ordered == (std::vector<std::uint32_t>{1u, 2u, 3u, 4u,
                                                 5u, 6u, 7u}) &&
          runtime.ReadDword(root + 20u) == 1u &&
          runtime.ReadDword(SecondRuntime::kHead) == nodes.at(1u) &&
          runtime.ReadDword(SecondRuntime::kHead + 8u) == nodes.at(7u) &&
          runtime.ReadDword(SecondRuntime::kTree + 12u) == input.size(),
          "second insert maintains balance, extremes and count");
    auto cursor = nodes.at(1u);
    std::vector<std::uint32_t> seen;
    while (cursor != SecondRuntime::kHead) {
        seen.push_back(runtime.ReadDword(cursor + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::AdvanceSecond(
            cursor, SecondRuntime::kSecondNil, runtime);
        Check(step.next == cursor, "second successor updates its cursor");
    }
    Check(seen == (std::vector<std::uint32_t>{1u, 2u, 3u, 4u,
                                               5u, 6u, 7u}),
          "second successor traverses all nodes to the head");
    runtime.words[SecondRuntime::kHead + 20u] = 0u;
    cursor = SecondRuntime::kHead;
    seen.clear();
    const auto from_end = pal4::EffectSystemEmbeddedTrees::PreviousSecond(
        cursor, SecondRuntime::kSecondNil, runtime);
    Check(cursor == nodes.at(7u) && from_end.next == cursor && from_end.result == cursor,
          "second predecessor decrements the red header to its maximum");
    while (cursor != SecondRuntime::kHead) {
        Check(seen.size() < nodes.size(), "second predecessor traversal terminates");
        seen.push_back(runtime.ReadDword(cursor + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::PreviousSecond(
            cursor, SecondRuntime::kSecondNil, runtime);
        Check(step.next == cursor, "second predecessor updates its cursor");
    }
    Check(seen == (std::vector<std::uint32_t>{7u, 6u, 5u, 4u, 3u, 2u, 1u}),
          "second predecessor visits left subtrees and climbs ancestors to the header");
}

void TestFourthLowerBoundEntry() {
    FourthRuntime runtime;
    Check(pal4::EffectSystemEmbeddedTrees::LowerBoundFourth(
              FourthRuntime::kTree, 4u, FourthRuntime::kFourthNil,
              runtime) == FourthRuntime::kHead,
          "empty fourth tree lower bound returns head");
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (const auto key : {4u, 2u, 6u})
        nodes[key] = runtime.AddFourth(key, 0x3000u + key * 16u);
    Check(pal4::EffectSystemEmbeddedTrees::LowerBoundFourth(
              FourthRuntime::kTree, 1u, FourthRuntime::kFourthNil,
              runtime) == nodes.at(2u) &&
          pal4::EffectSystemEmbeddedTrees::LowerBoundFourth(
              FourthRuntime::kTree, 4u, FourthRuntime::kFourthNil,
              runtime) == nodes.at(4u) &&
          pal4::EffectSystemEmbeddedTrees::LowerBoundFourth(
              FourthRuntime::kTree, 5u, FourthRuntime::kFourthNil,
              runtime) == nodes.at(6u) &&
          pal4::EffectSystemEmbeddedTrees::LowerBoundFourth(
              FourthRuntime::kTree, 7u, FourthRuntime::kFourthNil,
              runtime) == FourthRuntime::kHead,
          "fourth lower bound handles below, exact, between and above keys");
}

}  // namespace

int main() {
    TestInsertLookupAndCursor();
    TestEraseAndRanges();
    TestConstructor();
    TestFourthInsertAndClear();
    TestFifthEraseAndCursor();
    TestFifthConstructor();
    TestThirdPositionedInsertAndLinks();
    TestFourthNodeErasure();
    TestFourthPositionedInsert();
    TestFifthPositionedInsert();
    TestLaterTreePredecessors();
    TestFourthRotations();
    TestFirstExactAndLowerBoundEntries();
    TestSecondPositionedInsertAndCursors({4u, 2u, 6u, 1u, 3u, 5u, 7u});
    TestSecondPositionedInsertAndCursors({1u, 2u, 3u, 4u, 5u, 6u, 7u});
    TestFourthLowerBoundEntry();
}
