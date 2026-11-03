#include "pal4/ef_effect_system_key_tree.h"

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

struct Runtime final : pal4::IEfEffectSystemKeyInsertRuntime,
                       pal4::IEfEffectSystemKeyEraseRuntime {
    static constexpr auto kObject = 0x1000u;
    static constexpr auto kTree = kObject + 148u;
    static constexpr auto kHead = 0x2000u;
    static constexpr auto kKey = 0x3000u;
    static constexpr auto kNil = 0x93cde8u;
    static constexpr auto kResult = 0x5000u;

    std::map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil},
        {kHead + 8u, kHead}, {kHead + 16u, 0u},
        {kNil, kNil}, {kNil + 4u, 0u},
        {kNil + 8u, kNil}, {kNil + 16u, 1u}};
    bool enabled = true;
    bool duplicate_mode = false;
    std::uint32_t next_node = 0x4000u;
    std::uint32_t allocations = 0u;
    std::vector<std::uint32_t> frees;

    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        Check(address == kObject || address == kTree + 8u,
              "insert reads only activation and duplicate mode");
        return address == kObject ? enabled : duplicate_mode;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kObject,
              "erase and lookup read only activation byte");
        return enabled;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {
        Check(false, "key tree never writes text bytes");
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return ReadDword(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        WriteDword(address, value);
    }
    void WriteDword(std::uint32_t address,
                    std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t AllocateMemory(std::uint32_t bytes) override {
        Check(bytes == 20u, "key tree allocates 20-byte nodes");
        const auto node = next_node;
        next_node += 0x40u;
        ++allocations;
        return node;
    }
    std::uint32_t Allocate(std::uint32_t) override {
        Check(false, "erasure does not allocate nodes");
        return 0u;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return 0u;
    }
    std::uint32_t Call(pal4::ArenaSequenceCall,
                       std::initializer_list<std::uint32_t>) override {
        Check(false, "integer key tree has no native text service");
        return 0u;
    }
    std::uint32_t PrepareKey(std::uint32_t key) override {
        words[kKey] = key;
        return kKey;
    }
    std::uint32_t ScratchResultAddress() override { return kResult; }
};

int Validate(const Runtime& runtime, std::uint32_t node,
             std::uint32_t parent,
             std::vector<std::uint32_t>& keys) {
    if (node == Runtime::kNil) return 1;
    Check(runtime.ReadDword(node + 4u) == parent,
          "key tree parent points to its owner");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + 16u);
    Check(color <= 1u, "key tree color is red or black");
    if (color == 0u)
        Check(runtime.ReadDword(left + 16u) == 1u &&
              runtime.ReadDword(right + 16u) == 1u,
              "red key node has black children");
    const auto left_height = Validate(runtime, left, node, keys);
    keys.push_back(runtime.ReadDword(node + 12u));
    const auto right_height = Validate(runtime, right, node, keys);
    Check(left_height == right_height,
          "key tree keeps equal black heights");
    return left_height + static_cast<int>(color);
}

std::vector<std::uint32_t> Ordered(const Runtime& runtime) {
    std::vector<std::uint32_t> keys;
    const auto root = runtime.ReadDword(Runtime::kHead + 4u);
    if (root != Runtime::kNil)
        Check(runtime.ReadDword(root + 16u) == 1u,
              "key tree root is black");
    (void)Validate(runtime, root, Runtime::kHead, keys);
    Check(std::is_sorted(keys.begin(), keys.end()),
          "key tree sorts unsigned words");
    return keys;
}

std::uint32_t FindNode(const Runtime& runtime, std::uint32_t key) {
    auto node = runtime.ReadDword(Runtime::kHead + 4u);
    while (node != Runtime::kNil) {
        const auto current = runtime.ReadDword(node + 12u);
        if (current == key) return node;
        node = runtime.ReadDword(node + (key < current ? 0u : 8u));
    }
    return Runtime::kHead;
}

void TestActivationAndUnsignedInsertion() {
    Runtime runtime;
    runtime.enabled = false;
    Check(!pal4::EfEffectSystemKeyTree::InsertKey(
              Runtime::kObject, 3u, runtime) &&
          !pal4::EfEffectSystemKeyTree::ContainsKey(
              Runtime::kObject, 3u, Runtime::kNil, runtime) &&
          runtime.allocations == 0u,
          "inactive singleton ignores insert and lookup");
    runtime.enabled = true;
    for (const auto key : {0x80000000u, 1u, 0xffffffffu,
                           2u, 0u, 0x70000000u, 4u})
        Check(pal4::EfEffectSystemKeyTree::InsertKey(
                  Runtime::kObject, key, runtime),
              "new key inserts into singleton tree");
    Check(Ordered(runtime) == (std::vector<std::uint32_t>{
              0u, 1u, 2u, 4u, 0x70000000u,
              0x80000000u, 0xffffffffu}) &&
          runtime.ReadDword(Runtime::kTree + 12u) == 7u,
          "20-byte nodes use unsigned key order and retain count");
    for (const auto key : {0u, 2u, 0x80000000u, 0xffffffffu})
        Check(pal4::EfEffectSystemKeyTree::ContainsKey(
                  Runtime::kObject, key, Runtime::kNil, runtime),
              "exact key lookup finds inserted node");
    for (const auto key : {3u, 5u, 0x90000000u})
        Check(!pal4::EfEffectSystemKeyTree::ContainsKey(
                  Runtime::kObject, key, Runtime::kNil, runtime),
              "lookup rejects gaps in unsigned key order");
    const auto allocations = runtime.allocations;
    runtime.duplicate_mode = true;
    Check(!pal4::EfEffectSystemKeyTree::InsertKey(
              Runtime::kObject, 4u, runtime) &&
          runtime.allocations == allocations,
          "top-level add suppresses existing key even in duplicate mode");
}

void TestEraseEqualKeyRange() {
    Runtime runtime;
    for (const auto key : {4u, 2u, 6u, 1u, 3u, 5u, 7u})
        Check(pal4::EfEffectSystemKeyTree::InsertKey(
                  Runtime::kObject, key, runtime),
              "erase fixture inserts every key");
    const auto frees = runtime.frees.size();
    pal4::EfEffectSystemKeyTree::RemoveKey(
        Runtime::kObject, 9u, Runtime::kNil, runtime);
    Check(runtime.frees.size() == frees &&
          runtime.ReadDword(Runtime::kTree + 12u) == 7u,
          "missing key leaves tree untouched");
    for (const auto key : {4u, 1u, 7u, 3u, 2u, 5u, 6u}) {
        pal4::EfEffectSystemKeyTree::RemoveKey(
            Runtime::kObject, key, Runtime::kNil, runtime);
        Check(!pal4::EfEffectSystemKeyTree::ContainsKey(
                  Runtime::kObject, key, Runtime::kNil, runtime) &&
              runtime.frees.size() == 8u -
                  runtime.ReadDword(Runtime::kTree + 12u) - 1u &&
              Ordered(runtime).size() ==
                  runtime.ReadDword(Runtime::kTree + 12u),
              "erasure updates membership, count and red-black structure");
    }
    Check(runtime.ReadDword(Runtime::kHead) == Runtime::kHead &&
          runtime.ReadDword(Runtime::kHead + 4u) == Runtime::kNil &&
          runtime.ReadDword(Runtime::kHead + 8u) == Runtime::kHead,
          "erasing last key restores empty head links");

    Runtime duplicates;
    duplicates.duplicate_mode = true;
    for (int index = 0; index < 3; ++index) {
        (void)duplicates.PrepareKey(4u);
        Check(pal4::MonsterManagerTreeInsert::Insert(
                  Runtime::kTree, Runtime::kKey, duplicates).inserted,
              "multi-key fixture inserts duplicate nodes");
    }
    Check(Ordered(duplicates) ==
              (std::vector<std::uint32_t>{4u, 4u, 4u}),
          "duplicate fixture contains equal keys");
    pal4::EfEffectSystemKeyTree::RemoveKey(
        Runtime::kObject, 4u, Runtime::kNil, duplicates);
    Check(Ordered(duplicates).empty() &&
          duplicates.frees.size() == 3u &&
          duplicates.ReadDword(Runtime::kTree + 12u) == 0u,
          "equal-key removal clears entire duplicate range");

    Runtime middle;
    for (const auto key : {2u, 4u, 6u})
        Check(pal4::EfEffectSystemKeyTree::InsertKey(
                  Runtime::kObject, key, middle),
              "partial-range fixture inserts distinct keys");
    middle.duplicate_mode = true;
    for (int index = 0; index < 2; ++index) {
        (void)middle.PrepareKey(4u);
        Check(pal4::MonsterManagerTreeInsert::Insert(
                  Runtime::kTree, Runtime::kKey, middle).inserted,
              "partial-range fixture adds equal middle keys");
    }
    pal4::EfEffectSystemKeyTree::RemoveKey(
        Runtime::kObject, 4u, Runtime::kNil, middle);
    Check(Ordered(middle) == (std::vector<std::uint32_t>{2u, 6u}) &&
          middle.frees.size() == 3u &&
          middle.ReadDword(Runtime::kTree + 12u) == 2u,
          "partial equal-key range preserves smaller and larger keys");

    duplicates.enabled = false;
    pal4::EfEffectSystemKeyTree::RemoveKey(
        Runtime::kObject, 4u, Runtime::kNil, duplicates);
    Check(duplicates.frees.size() == 3u,
          "inactive singleton skips key removal");
}

void TestDirectNodeErasure() {
    Runtime runtime;
    for (const auto key : {8u, 4u, 12u, 2u, 6u, 10u, 14u,
                           1u, 3u, 5u, 7u, 9u, 11u, 13u, 15u})
        Check(pal4::EfEffectSystemKeyTree::InsertKey(
                  Runtime::kObject, key, runtime),
              "direct erase fixture inserts distinct keys");

    for (const auto key : {8u, 1u, 15u, 4u, 12u, 2u, 6u, 10u,
                           14u, 3u, 5u, 7u, 9u, 11u, 13u}) {
        const auto before = Ordered(runtime);
        const auto successor = std::upper_bound(before.begin(), before.end(),
                                                 key);
        const auto node = FindNode(runtime, key);
        Check(node != Runtime::kHead,
              "direct erase finds the node to remove");
        const auto frees = runtime.frees.size();
        Check(pal4::EfEffectSystemKeyTree::EraseNode(
                  Runtime::kTree, Runtime::kResult, node,
                  Runtime::kNil, runtime) == Runtime::kResult &&
              runtime.frees.size() == frees + 1u &&
              runtime.frees.back() == node &&
              runtime.ReadDword(Runtime::kTree + 12u) ==
                  before.size() - 1u &&
              (successor == before.end()
                   ? runtime.ReadDword(Runtime::kResult) == Runtime::kHead
                   : runtime.ReadDword(
                         runtime.ReadDword(Runtime::kResult) + 12u) ==
                         *successor),
              "direct erase returns next iterator and frees only its node");
        const auto after = Ordered(runtime);
        Check(after.size() + 1u == before.size() &&
              !std::binary_search(after.begin(), after.end(), key),
              "direct erase maintains red-black shape and key ordering");
    }
}

void TestPositionedInsertion() {
    Runtime runtime;
    auto input = runtime.PrepareKey(8u);
    Check(pal4::EfEffectSystemKeyTree::InsertAt(
              Runtime::kTree, Runtime::kResult, Runtime::kNil,
              Runtime::kHead, input, runtime) == Runtime::kResult,
          "positioned insertion returns result address");
    const auto root = runtime.ReadDword(Runtime::kResult);
    Check(root != Runtime::kNil &&
          runtime.ReadDword(root + 12u) == 8u &&
          runtime.ReadDword(Runtime::kHead + 4u) == root,
          "positioned insertion links first node as root");

    input = runtime.PrepareKey(4u);
    (void)pal4::EfEffectSystemKeyTree::InsertAt(
        Runtime::kTree, Runtime::kResult, Runtime::kNil,
        root, input, runtime);
    input = runtime.PrepareKey(12u);
    (void)pal4::EfEffectSystemKeyTree::InsertAt(
        Runtime::kTree, Runtime::kResult, Runtime::kNil,
        root, input, runtime);
    Check(Ordered(runtime) == (std::vector<std::uint32_t>{4u, 8u, 12u}) &&
          runtime.ReadDword(Runtime::kTree + 12u) == 3u &&
          runtime.allocations == 3u,
          "positioned insertion copies keys and preserves red-black links");
}

void TestRightFirstSubtreeDeletion() {
    Runtime runtime;
    constexpr auto root = 0x4000u;
    constexpr auto right = 0x4040u;
    constexpr auto left = 0x4080u;
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[right] = Runtime::kNil;
    runtime.words[right + 8u] = Runtime::kNil;
    runtime.words[left] = Runtime::kNil;
    runtime.words[left + 8u] = Runtime::kNil;
    Check(pal4::EfEffectSystemKeyTree::DeleteSubtree(
              root, Runtime::kNil, runtime) == Runtime::kNil &&
          runtime.frees == (std::vector<std::uint32_t>{right, root, left}),
          "key subtree releases right branches before left-spine nodes");
    runtime.frees.clear();
    Check(pal4::EfEffectSystemKeyTree::DeleteSubtree(
              Runtime::kNil, Runtime::kNil, runtime) == Runtime::kNil &&
          runtime.frees.empty(),
          "nil subtree requires no release");
}

void TestBoundsAndReverseCursor() {
    Runtime runtime;
    Check(pal4::EfEffectSystemKeyTree::LowerBound(
              Runtime::kTree, 3u, Runtime::kNil, runtime) ==
                  Runtime::kHead &&
          pal4::EfEffectSystemKeyTree::UpperBound(
              Runtime::kTree, 3u, Runtime::kNil, runtime) ==
                  Runtime::kHead,
          "empty key tree has header as both bounds");
    for (const auto key : {4u, 2u, 6u, 1u, 3u, 5u, 7u})
        Check(pal4::EfEffectSystemKeyTree::InsertKey(
                  Runtime::kObject, key, runtime),
              "bound fixture inserts key");
    const auto lower = pal4::EfEffectSystemKeyTree::LowerBound(
        Runtime::kTree, 3u, Runtime::kNil, runtime);
    const auto gap = pal4::EfEffectSystemKeyTree::LowerBound(
        Runtime::kTree, 8u, Runtime::kNil, runtime);
    const auto upper = pal4::EfEffectSystemKeyTree::UpperBound(
        Runtime::kTree, 4u, Runtime::kNil, runtime);
    Check(runtime.ReadDword(lower + 12u) == 3u &&
          gap == Runtime::kHead &&
          runtime.ReadDword(upper + 12u) == 5u &&
          pal4::EfEffectSystemKeyTree::UpperBound(
              Runtime::kTree, 7u, Runtime::kNil, runtime) ==
                  Runtime::kHead,
          "lower and upper bounds handle hits, gaps and maximum");

    auto iterator = Runtime::kHead;
    std::vector<std::uint32_t> reverse;
    for (int index = 0; index < 7; ++index) {
        (void)pal4::EfEffectSystemKeyTree::Previous(
            iterator, Runtime::kNil, runtime);
        reverse.push_back(runtime.ReadDword(iterator + 12u));
    }
    (void)pal4::EfEffectSystemKeyTree::Previous(
        iterator, Runtime::kNil, runtime);
    Check(reverse == (std::vector<std::uint32_t>{
              7u, 6u, 5u, 4u, 3u, 2u, 1u}) &&
          iterator == Runtime::kHead,
          "reverse cursor traverses all keys and reaches header");
}

}  // namespace

int main() {
    TestActivationAndUnsignedInsertion();
    TestEraseEqualKeyRange();
    TestDirectNodeErasure();
    TestPositionedInsertion();
    TestRightFirstSubtreeDeletion();
    TestBoundsAndReverseCursor();
}
