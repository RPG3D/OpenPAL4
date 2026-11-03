#include "pal4/effect_system.h"
#include "pal4/arena_sequence_resources.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

using Kind = pal4::EffectSystemEmbeddedTreeKind;
using Cleanup = std::uint32_t (*)(
    std::uint32_t, pal4::IEffectSystemEmbeddedTreeRuntime&);

constexpr std::array<Cleanup, 5> kCleanup{
    pal4::EffectSystemEmbeddedTrees::CleanupFirstDirect,
    pal4::EffectSystemEmbeddedTrees::CleanupSecondDirect,
    pal4::EffectSystemEmbeddedTrees::CleanupThirdDirect,
    pal4::EffectSystemEmbeddedTrees::CleanupFourthDirect,
    pal4::EffectSystemEmbeddedTrees::CleanupFifthDirect,
};
constexpr std::array<Kind, 5> kKinds{
    Kind::First, Kind::Second, Kind::Third, Kind::Fourth, Kind::Fifth,
};
constexpr std::array<std::uint32_t, 5> kNils{
    0x93cdb8u, 0x93cdc0u, 0x93cdc8u, 0x93cdd0u, 0x93cdd8u,
};

struct TreeRuntime final : pal4::IEffectSystemEmbeddedTreeRuntime {
    static constexpr std::uint32_t kTree = 0x1000u;
    static constexpr std::uint32_t kHead = 0x2000u;
    static constexpr std::uint32_t kNode1 = 0x3000u;
    static constexpr std::uint32_t kNode2 = 0x3100u;

    std::map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, 0u}, {kHead + 8u, kHead}};
    std::map<Kind, std::uint32_t> nils;
    std::map<Kind, std::uint32_t> references;
    std::map<std::uint32_t, std::uint32_t> next;
    std::vector<std::string> calls;

    TreeRuntime() {
        for (std::size_t index = 0; index < kKinds.size(); ++index) {
            nils[kKinds[index]] = kNils[index];
            references[kKinds[index]] = 2u;
        }
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SharedNil(Kind kind) override { return nils.at(kind); }
    void ClearSharedNil(Kind kind) override {
        calls.push_back("clear " + std::to_string(static_cast<int>(kind)));
        nils[kind] = 0u;
    }
    std::uint32_t ReleaseSharedNilReference(Kind kind) override {
        calls.push_back("release " + std::to_string(static_cast<int>(kind)));
        return --references[kind];
    }
    void DeleteSubtree(Kind kind, std::uint32_t root) override {
        calls.push_back("delete " + std::to_string(static_cast<int>(kind)) +
                        " " + std::to_string(root));
    }
    void ReadFirstElement(std::uint32_t) override {
        Check(false, "direct cleanup omits earlier variant's readback");
    }
    std::uint32_t Next(Kind kind, std::uint32_t node) override {
        calls.push_back("next " + std::to_string(static_cast<int>(kind)) +
                        " " + std::to_string(node));
        return next.at(node);
    }
    void RemoveNode(Kind kind, std::uint32_t tree,
                    std::uint32_t node) override {
        Check(tree == kTree, "erase uses the same tree");
        calls.push_back("remove " + std::to_string(static_cast<int>(kind)) +
                        " " + std::to_string(node));
        words[kHead] = next.at(node);
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        calls.push_back("free " + std::to_string(pointer));
        return pointer + 1u;
    }
};

struct ConstructRuntime final :
    pal4::IEffectSystemEmbeddedTreeConstructRuntime {
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void InitializeTree(std::uint32_t tree) override {
        Check(tree == TreeRuntime::kTree &&
              bytes.at(tree) == 0xa1u &&
              bytes.at(tree + 1u) == 0xb2u &&
              bytes.at(tree + 8u) == 1u,
              "tree helper sees all three copied flags");
        calls.push_back("initialize");
    }
};

struct StorageRuntime final : pal4::IEffectSystemTreeStorageRuntime {
    using Kind = pal4::EffectSystemEmbeddedTreeKind;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t nil = 0u;
    std::uint32_t references = 0u;
    std::uint32_t next_allocation = 0x6000u;
    Kind expected_kind = Kind::First;
    std::uint32_t expected_size = 28u;

    std::uint32_t SharedNil(Kind kind) override {
        Check(kind == expected_kind, "storage uses its own tree globals");
        return nil;
    }
    void SetSharedNil(Kind kind, std::uint32_t value) override {
        Check(kind == expected_kind, "storage sets its own tree nil");
        calls.push_back("set nil");
        nil = value;
    }
    std::uint32_t SharedNilReferences(Kind kind) override {
        Check(kind == expected_kind, "storage reads its own references");
        return references;
    }
    void SetSharedNilReferences(Kind kind, std::uint32_t value) override {
        Check(kind == expected_kind, "storage writes its own references");
        calls.push_back("set references");
        references = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        Check(size == expected_size, "nil and head use the native node size");
        calls.push_back("allocate");
        const auto allocation = next_allocation;
        next_allocation += 0x40u;
        return allocation;
    }
    void Free(std::uint32_t pointer) override {
        calls.push_back("free " + std::to_string(pointer));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
};

struct InsertRuntime final : pal4::IEffectSystemFirstTreeInsertRuntime,
                             pal4::IArenaSequenceRuntime {
    static constexpr std::uint32_t kNil = kNils[0];
    std::map<std::uint32_t, std::uint32_t> words{
        {TreeRuntime::kTree + 4u, TreeRuntime::kHead},
        {TreeRuntime::kTree + 12u, 0u},
        {TreeRuntime::kHead, TreeRuntime::kHead},
        {TreeRuntime::kHead + 4u, kNil},
        {TreeRuntime::kHead + 8u, TreeRuntime::kHead},
        {kNil, kNil}, {kNil + 4u, 0u}, {kNil + 8u, kNil},
        {kNil + 24u, 1u}};
    bool allow_duplicates = false;
    std::uint32_t next_node = 0x4000u;
    std::uint32_t allocations = 0u;
    std::vector<std::uint32_t> frees;

    std::uint32_t SharedNil() const override { return kNil; }
    std::uint32_t ReadDword(std::uint32_t address) const override {
        return words.at(address);
    }
    std::uint8_t ReadByte(std::uint32_t address) const override {
        Check(address == TreeRuntime::kTree + 8u,
              "insertion reads the tree duplicate-mode byte");
        return allow_duplicates ? 1u : 0u;
    }
    std::uint8_t ReadByte(std::uint32_t) override {
        Check(false, "pure effect node deletion never reads text bytes");
        return 0u;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {
        Check(false, "pure effect node deletion never writes text bytes");
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
        Check(size == 28u, "first tree allocates 28-byte nodes");
        const auto node = next_node;
        next_node += 0x40u;
        ++allocations;
        return node;
    }
    std::uint32_t Allocate(std::uint32_t) override {
        Check(false, "erasure performs no allocation");
        return 0u;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return 0u;
    }
    std::uint32_t Call(pal4::ArenaSequenceCall,
                       std::initializer_list<std::uint32_t>) override {
        Check(false, "pure effect tree needs no native service call");
        return 0u;
    }
    void AddRecord(std::uint32_t address, std::uint32_t key) {
        words[address] = key;
        words[address + 4u] = key ^ 0x11111111u;
        words[address + 8u] = key ^ 0x22222222u;
    }
};

int ValidateInsertTree(const InsertRuntime& runtime, std::uint32_t node,
                       std::uint32_t parent,
                       std::vector<std::uint32_t>& keys,
                       std::uint32_t nil = InsertRuntime::kNil,
                       std::uint32_t color_offset = 24u) {
    if (node == nil) return 1;
    Check(runtime.ReadDword(node + 4u) == parent,
          "inserted node has correct parent");
    const auto left = runtime.ReadDword(node);
    const auto right = runtime.ReadDword(node + 8u);
    const auto color = runtime.ReadDword(node + color_offset);
    Check(color <= 1u, "node color is red or black");
    if (color == 0u)
        Check(runtime.ReadDword(left + color_offset) == 1u &&
              runtime.ReadDword(right + color_offset) == 1u,
              "red node has black children");
    const auto left_height = ValidateInsertTree(
        runtime, left, node, keys, nil, color_offset);
    keys.push_back(runtime.ReadDword(node + 12u));
    const auto right_height = ValidateInsertTree(
        runtime, right, node, keys, nil, color_offset);
    Check(left_height == right_height, "tree has equal black heights");
    return left_height + static_cast<int>(color);
}

void ConvertToSecondTree(
    InsertRuntime& runtime,
    const std::map<std::uint32_t, std::uint32_t>& nodes) {
    constexpr auto nil = kNils[1];
    runtime.words[nil] = nil;
    runtime.words[nil + 4u] = 0u;
    runtime.words[nil + 8u] = nil;
    runtime.words[nil + 20u] = 1u;
    for (const auto& [key, node] : nodes) {
        (void)key;
        for (const auto offset : {0u, 8u}) {
            if (runtime.words.at(node + offset) == InsertRuntime::kNil)
                runtime.words[node + offset] = nil;
        }
        runtime.words[node + 20u] = runtime.words.at(node + 24u);
    }
}

void TestPopulatedTrees() {
    for (std::size_t index = 0; index < kCleanup.size(); ++index) {
        TreeRuntime runtime;
        const auto kind = kKinds[index];
        runtime.words[TreeRuntime::kTree + 12u] = 2u;
        runtime.words[TreeRuntime::kHead] = TreeRuntime::kNode1;
        runtime.words[TreeRuntime::kHead + 4u] = TreeRuntime::kNode1;
        runtime.words[TreeRuntime::kHead + 8u] = TreeRuntime::kNode2;
        Check(kCleanup[index](TreeRuntime::kTree, runtime) == 1u &&
              runtime.words.at(TreeRuntime::kTree + 4u) == 0u &&
              runtime.words.at(TreeRuntime::kTree + 12u) == 0u &&
              runtime.words.at(TreeRuntime::kHead) == TreeRuntime::kHead &&
              runtime.words.at(TreeRuntime::kHead + 4u) == kNils[index] &&
              runtime.words.at(TreeRuntime::kHead + 8u) == TreeRuntime::kHead &&
              runtime.references.at(kind) == 1u &&
              runtime.calls == (std::vector<std::string>{
                  "delete " + std::to_string(index) + " 12288",
                  "free 8192", "release " + std::to_string(index)}),
              "each direct destructor clears its own populated tree");
    }
}

void TestFallbackAndLastReference() {
    for (std::size_t index = 0; index < kCleanup.size(); ++index) {
        TreeRuntime runtime;
        runtime.words[TreeRuntime::kHead] = TreeRuntime::kNode1;
        runtime.next[TreeRuntime::kNode1] = TreeRuntime::kNode2;
        runtime.next[TreeRuntime::kNode2] = TreeRuntime::kHead;
        runtime.references[kKinds[index]] = 1u;
        Check(kCleanup[index](TreeRuntime::kTree, runtime) ==
                  kNils[index] + 1u &&
              runtime.nils.at(kKinds[index]) == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "next " + std::to_string(index) + " 12288",
                  "remove " + std::to_string(index) + " 12288",
                  "next " + std::to_string(index) + " 12544",
                  "remove " + std::to_string(index) + " 12544",
                  "free 8192", "release " + std::to_string(index),
                  "clear " + std::to_string(index),
                  "free " + std::to_string(kNils[index])}),
              "fallback removes successors and frees only the last nil");
    }
}

void TestConstructor() {
    ConstructRuntime runtime;
    constexpr auto tree = TreeRuntime::kTree;
    Check(pal4::EffectSystemEmbeddedTrees::InitializeFirst(
              tree, 0xa1u, 0xb2u, 1u, runtime) == tree &&
          runtime.calls == (std::vector<std::string>{
              "write 4096", "write 4104", "write 4097", "initialize"}),
          "first-tree constructor copies flags in assembly order");
}

void TestFirstTreeInsertion() {
    InsertRuntime runtime;
    const std::vector<std::uint32_t> input{
        0x80000000u, 1u, 0xffffffffu, 2u, 0x90000000u,
        3u, 0x70000000u, 4u};
    std::uint32_t first_node = 0u;
    for (std::size_t index = 0; index < input.size(); ++index) {
        const auto record = 0x3000u + static_cast<std::uint32_t>(index) * 16u;
        runtime.AddRecord(record, input[index]);
        const auto result = pal4::EffectSystemEmbeddedTrees::InsertFirst(
            TreeRuntime::kTree, record, runtime);
        Check(result.inserted && result.node != InsertRuntime::kNil &&
              runtime.ReadDword(result.node + 12u) == input[index] &&
              runtime.ReadDword(result.node + 16u) ==
                  (input[index] ^ 0x11111111u) &&
              runtime.ReadDword(result.node + 20u) ==
                  (input[index] ^ 0x22222222u),
              "insertion copies the complete three-word record");
        if (index == 0u) first_node = result.node;
    }
    std::vector<std::uint32_t> ordered;
    const auto root = runtime.ReadDword(TreeRuntime::kHead + 4u);
    Check(runtime.ReadDword(root + 24u) == 1u,
          "insert repair leaves a black root");
    (void)ValidateInsertTree(runtime, root, TreeRuntime::kHead, ordered);
    Check(ordered == (std::vector<std::uint32_t>{
              1u, 2u, 3u, 4u, 0x70000000u, 0x80000000u,
              0x90000000u, 0xffffffffu}) &&
          runtime.ReadDword(TreeRuntime::kTree + 12u) == input.size(),
          "first tree sorts unsigned keys and rebalances");

    runtime.AddRecord(0x3200u, 0x80000000u);
    const auto duplicate = pal4::EffectSystemEmbeddedTrees::InsertFirst(
        TreeRuntime::kTree, 0x3200u, runtime);
    Check(!duplicate.inserted && duplicate.node == first_node &&
          runtime.allocations == input.size(),
          "unique mode returns the existing node without allocation");

    runtime.allow_duplicates = true;
    const auto repeated = pal4::EffectSystemEmbeddedTrees::InsertFirst(
        TreeRuntime::kTree, 0x3200u, runtime);
    Check(repeated.inserted && repeated.node != first_node &&
          runtime.allocations == input.size() + 1u &&
          runtime.ReadDword(TreeRuntime::kTree + 12u) == input.size() + 1u,
          "multi mode creates a second node with the same key");
}

void TestFirstTreeErasure() {
    InsertRuntime runtime;
    const std::vector<std::uint32_t> input{
        8u, 4u, 12u, 2u, 6u, 10u, 14u, 1u, 3u, 5u, 7u, 9u, 11u, 13u, 15u};
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (std::size_t index = 0; index < input.size(); ++index) {
        const auto record = 0x3000u + static_cast<std::uint32_t>(index) * 16u;
        runtime.AddRecord(record, input[index]);
        const auto inserted = pal4::EffectSystemEmbeddedTrees::InsertFirst(
            TreeRuntime::kTree, record, runtime);
        Check(inserted.inserted, "erasure fixture inserts each key");
        nodes[input[index]] = inserted.node;
    }
    const std::vector<std::uint32_t> removal_order{
        8u, 1u, 15u, 6u, 4u, 12u, 3u, 2u,
        10u, 14u, 5u, 7u, 9u, 11u, 13u};
    for (const auto key : removal_order) {
        pal4::EffectSystemEmbeddedTrees::EraseFirstNode(
            TreeRuntime::kTree, 0x5000u, nodes.at(key),
            InsertRuntime::kNil, runtime);
        Check(runtime.frees.back() == nodes.at(key) &&
              runtime.ReadDword(TreeRuntime::kTree + 12u) ==
                  input.size() - runtime.frees.size(),
              "erasure frees the chosen node and decrements count");
        nodes.erase(key);
        std::vector<std::uint32_t> ordered;
        const auto root = runtime.ReadDword(TreeRuntime::kHead + 4u);
        (void)ValidateInsertTree(runtime, root, TreeRuntime::kHead, ordered);
        std::vector<std::uint32_t> expected;
        for (const auto& [remaining, pointer] : nodes) {
            (void)pointer;
            expected.push_back(remaining);
        }
        Check(ordered == expected &&
              runtime.ReadDword(TreeRuntime::kHead) ==
                  (nodes.empty() ? TreeRuntime::kHead
                                 : nodes.begin()->second) &&
              runtime.ReadDword(TreeRuntime::kHead + 8u) ==
                  (nodes.empty() ? TreeRuntime::kHead
                                 : nodes.rbegin()->second),
              "each deletion keeps ordered links, extremes and red-black balance");
    }
}

void TestFirstTreeRangeAndSuccessor() {
    InsertRuntime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (std::uint32_t key = 1u; key <= 7u; ++key) {
        const auto record = 0x3000u + key * 16u;
        runtime.AddRecord(record, key);
        nodes[key] = pal4::EffectSystemEmbeddedTrees::InsertFirst(
            TreeRuntime::kTree, record, runtime).node;
    }
    auto iterator = runtime.ReadDword(TreeRuntime::kHead);
    std::vector<std::uint32_t> seen;
    while (iterator != TreeRuntime::kHead) {
        seen.push_back(runtime.ReadDword(iterator + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::AdvanceFirst(
            iterator, InsertRuntime::kNil, runtime);
        Check(iterator == step.next, "successor updates the iterator");
    }
    Check(seen == (std::vector<std::uint32_t>{1u, 2u, 3u, 4u,
                                             5u, 6u, 7u}),
          "successor visits first-tree nodes in order");

    pal4::EffectSystemEmbeddedTrees::EraseFirstRange(
        TreeRuntime::kTree, 0x5000u, nodes.at(2u), nodes.at(5u),
        InsertRuntime::kNil, runtime);
    std::vector<std::uint32_t> ordered;
    (void)ValidateInsertTree(
        runtime, runtime.ReadDword(TreeRuntime::kHead + 4u),
        TreeRuntime::kHead, ordered);
    Check(ordered == (std::vector<std::uint32_t>{1u, 5u, 6u, 7u}) &&
          runtime.ReadDword(0x5000u) == nodes.at(5u) &&
          runtime.ReadDword(TreeRuntime::kTree + 12u) == 4u &&
          runtime.frees.size() == 3u,
          "partial range erases through its exclusive end");

    pal4::EffectSystemEmbeddedTrees::EraseFirstRange(
        TreeRuntime::kTree, 0x5000u,
        runtime.ReadDword(TreeRuntime::kHead), TreeRuntime::kHead,
        InsertRuntime::kNil, runtime);
    Check(runtime.ReadDword(0x5000u) == TreeRuntime::kHead &&
          runtime.ReadDword(TreeRuntime::kTree + 12u) == 0u &&
          runtime.ReadDword(TreeRuntime::kHead) == TreeRuntime::kHead &&
          runtime.ReadDword(TreeRuntime::kHead + 4u) == InsertRuntime::kNil &&
          runtime.ReadDword(TreeRuntime::kHead + 8u) == TreeRuntime::kHead &&
          runtime.frees.size() == 7u,
          "whole range takes the right-first subtree cleanup path");
}

void TestSecondConstructor() {
    ConstructRuntime runtime;
    constexpr auto tree = TreeRuntime::kTree;
    Check(pal4::EffectSystemEmbeddedTrees::InitializeSecond(
              tree, 0xa1u, 0xb2u, 1u, runtime) == tree &&
          runtime.calls == (std::vector<std::string>{
              "write 4096", "write 4104", "write 4097", "initialize"}),
          "second-tree constructor preserves flag-copy order");
}

void TestSecondTreeErasure() {
    InsertRuntime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    const std::vector<std::uint32_t> keys{
        8u, 4u, 12u, 2u, 6u, 10u, 14u, 1u, 3u, 5u,
        7u, 9u, 11u, 13u, 15u};
    for (std::size_t index = 0; index < keys.size(); ++index) {
        const auto record = 0x3000u + static_cast<std::uint32_t>(index) * 16u;
        runtime.AddRecord(record, keys[index]);
        nodes[keys[index]] = pal4::EffectSystemEmbeddedTrees::InsertFirst(
            TreeRuntime::kTree, record, runtime).node;
    }
    ConvertToSecondTree(runtime, nodes);
    constexpr auto nil = kNils[1];
    for (const auto key : keys) {
        pal4::EffectSystemEmbeddedTrees::EraseSecondNode(
            TreeRuntime::kTree, 0x5000u, nodes.at(key), nil, runtime);
        Check(runtime.frees.back() == nodes.at(key),
              "second-tree erase frees the requested 24-byte node");
        nodes.erase(key);
        std::vector<std::uint32_t> actual;
        (void)ValidateInsertTree(
            runtime, runtime.ReadDword(TreeRuntime::kHead + 4u),
            TreeRuntime::kHead, actual, nil, 20u);
        std::vector<std::uint32_t> expected;
        for (const auto& [remaining, pointer] : nodes) {
            (void)pointer;
            expected.push_back(remaining);
        }
        Check(actual == expected &&
              runtime.ReadDword(TreeRuntime::kTree + 12u) == nodes.size() &&
              runtime.ReadDword(TreeRuntime::kHead) ==
                  (nodes.empty() ? TreeRuntime::kHead
                                 : nodes.begin()->second) &&
              runtime.ReadDword(TreeRuntime::kHead + 8u) ==
                  (nodes.empty() ? TreeRuntime::kHead
                                 : nodes.rbegin()->second),
              "second-tree erase maintains order, extremes and black heights");
    }
}

void TestSecondTreeRangeAndSuccessor() {
    InsertRuntime runtime;
    std::map<std::uint32_t, std::uint32_t> nodes;
    for (std::uint32_t key = 1u; key <= 7u; ++key) {
        const auto record = 0x3000u + key * 16u;
        runtime.AddRecord(record, key);
        nodes[key] = pal4::EffectSystemEmbeddedTrees::InsertFirst(
            TreeRuntime::kTree, record, runtime).node;
    }
    ConvertToSecondTree(runtime, nodes);
    constexpr auto nil = kNils[1];
    auto iterator = runtime.ReadDword(TreeRuntime::kHead);
    std::vector<std::uint32_t> seen;
    while (iterator != TreeRuntime::kHead) {
        seen.push_back(runtime.ReadDword(iterator + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::AdvanceSecond(
            iterator, nil, runtime);
        Check(iterator == step.next,
              "second-tree successor updates iterator");
    }
    Check(seen == (std::vector<std::uint32_t>{
              1u, 2u, 3u, 4u, 5u, 6u, 7u}),
          "second-tree successor traverses every key");

    pal4::EffectSystemEmbeddedTrees::EraseSecondRange(
        TreeRuntime::kTree, 0x5000u, nodes.at(2u), nodes.at(5u),
        nil, runtime);
    std::vector<std::uint32_t> ordered;
    (void)ValidateInsertTree(
        runtime, runtime.ReadDword(TreeRuntime::kHead + 4u),
        TreeRuntime::kHead, ordered, nil, 20u);
    Check(ordered == (std::vector<std::uint32_t>{1u, 5u, 6u, 7u}) &&
          runtime.ReadDword(0x5000u) == nodes.at(5u) &&
          runtime.frees.size() == 3u,
          "second-tree partial range uses exclusive end");

    pal4::EffectSystemEmbeddedTrees::EraseSecondRange(
        TreeRuntime::kTree, 0x5000u,
        runtime.ReadDword(TreeRuntime::kHead), TreeRuntime::kHead,
        nil, runtime);
    Check(runtime.ReadDword(0x5000u) == TreeRuntime::kHead &&
          runtime.ReadDword(TreeRuntime::kTree + 12u) == 0u &&
          runtime.ReadDword(TreeRuntime::kHead + 4u) == nil &&
          runtime.ReadDword(TreeRuntime::kHead) == TreeRuntime::kHead &&
          runtime.ReadDword(TreeRuntime::kHead + 8u) == TreeRuntime::kHead &&
          runtime.frees.size() == 7u,
          "second-tree complete range releases all nodes");
}

void TestFirstTreeStorage() {
    StorageRuntime new_nil;
    constexpr auto tree = TreeRuntime::kTree;
    constexpr auto candidate = 0x6000u;
    constexpr auto head = 0x6040u;
    Check(pal4::EffectSystemEmbeddedTrees::InitializeFirstStorage(
              tree, new_nil) == head &&
          new_nil.nil == candidate && new_nil.references == 1u &&
          new_nil.words.at(candidate) == 0u &&
          new_nil.words.at(candidate + 4u) == 0u &&
          new_nil.words.at(candidate + 8u) == 0u &&
          new_nil.words.at(candidate + 24u) == 1u &&
          new_nil.words.at(head) == head &&
          new_nil.words.at(head + 4u) == candidate &&
          new_nil.words.at(head + 8u) == head &&
          new_nil.words.at(head + 24u) == 0u &&
          new_nil.words.at(tree + 4u) == head &&
          new_nil.words.at(tree + 12u) == 0u &&
          new_nil.calls == (std::vector<std::string>{
              "allocate", "set nil", "set references", "allocate"}),
          "first owner creates shared nil then tree head");

    StorageRuntime existing_nil;
    existing_nil.nil = 0x7000u;
    existing_nil.references = 2u;
    Check(pal4::EffectSystemEmbeddedTrees::InitializeFirstStorage(
              tree, existing_nil) == head &&
          existing_nil.nil == 0x7000u &&
          existing_nil.references == 3u &&
          existing_nil.words.at(head + 4u) == 0x7000u &&
          existing_nil.calls == (std::vector<std::string>{
              "allocate", "set references", "free 24576", "allocate"}),
          "later owner frees candidate after increasing shared reference");
}

void TestFirstTreeInsertHelper() {
    InsertRuntime runtime;
    runtime.AddRecord(0x3000u, 0x80000000u);
    const auto node = pal4::EffectSystemEmbeddedTrees::InsertFirstAt(
        TreeRuntime::kTree, InsertRuntime::kNil,
        TreeRuntime::kHead, 0x3000u, runtime);
    Check(node == 0x4000u &&
          runtime.ReadDword(node) == InsertRuntime::kNil &&
          runtime.ReadDword(node + 4u) == TreeRuntime::kHead &&
          runtime.ReadDword(node + 8u) == InsertRuntime::kNil &&
          runtime.ReadDword(node + 12u) == 0x80000000u &&
          runtime.ReadDword(node + 16u) == 0x91111111u &&
          runtime.ReadDword(node + 20u) == 0xa2222222u &&
          runtime.ReadDword(node + 24u) == 1u &&
          runtime.ReadDword(TreeRuntime::kHead) == node &&
          runtime.ReadDword(TreeRuntime::kHead + 4u) == node &&
          runtime.ReadDword(TreeRuntime::kHead + 8u) == node &&
          runtime.ReadDword(TreeRuntime::kTree + 12u) == 1u,
          "known-position insertion links and repairs a 28-byte root");
}

void TestFirstTreeRotationsAndMinimum() {
    InsertRuntime runtime;
    constexpr std::uint32_t root = 0x5000u;
    constexpr std::uint32_t right = 0x5100u;
    constexpr std::uint32_t moved = 0x5200u;
    runtime.words[TreeRuntime::kHead + 4u] = root;
    runtime.words[root] = InsertRuntime::kNil;
    runtime.words[root + 4u] = TreeRuntime::kHead;
    runtime.words[root + 8u] = right;
    runtime.words[right] = moved;
    runtime.words[right + 4u] = root;
    runtime.words[right + 8u] = InsertRuntime::kNil;
    runtime.words[moved] = InsertRuntime::kNil;
    runtime.words[moved + 4u] = right;
    runtime.words[moved + 8u] = InsertRuntime::kNil;
    Check(pal4::EffectSystemEmbeddedTrees::FirstMinimumFromLink(
              root + 8u, InsertRuntime::kNil, runtime) == moved,
          "minimum follows left links from the supplied child link");
    Check(pal4::EffectSystemEmbeddedTrees::PromoteFirstRightChild(
              TreeRuntime::kTree, root, runtime) == right &&
          runtime.ReadDword(TreeRuntime::kHead + 4u) == right &&
          runtime.ReadDword(right) == root &&
          runtime.ReadDword(root + 8u) == moved &&
          runtime.ReadDword(moved + 4u) == root,
          "right promotion updates root and moved-child parents");
    Check(pal4::EffectSystemEmbeddedTrees::PromoteFirstLeftChild(
              TreeRuntime::kTree, right, runtime) == root &&
          runtime.ReadDword(TreeRuntime::kHead + 4u) == root &&
          runtime.ReadDword(root + 8u) == right &&
          runtime.ReadDword(right) == moved &&
          runtime.ReadDword(moved + 4u) == right,
          "left promotion restores original root and links");
}

void TestFirstTreeSubtreeDeletion() {
    InsertRuntime runtime;
    constexpr std::uint32_t root = 0x5000u;
    constexpr std::uint32_t left = 0x5100u;
    constexpr std::uint32_t right = 0x5200u;
    runtime.words[root] = left;
    runtime.words[root + 8u] = right;
    runtime.words[left] = InsertRuntime::kNil;
    runtime.words[left + 8u] = InsertRuntime::kNil;
    runtime.words[right] = InsertRuntime::kNil;
    runtime.words[right + 8u] = InsertRuntime::kNil;
    Check(pal4::EffectSystemEmbeddedTrees::DeleteFirstSubtree(
              root, InsertRuntime::kNil, runtime) == InsertRuntime::kNil &&
          runtime.frees == (std::vector<std::uint32_t>{
              right, root, left}),
          "subtree deletion visits right branch before left spine");
    const auto frees = runtime.frees.size();
    Check(pal4::EffectSystemEmbeddedTrees::DeleteFirstSubtree(
              InsertRuntime::kNil, InsertRuntime::kNil, runtime) ==
              InsertRuntime::kNil &&
          runtime.frees.size() == frees,
          "empty subtree deletes no nodes");
}

void TestLaterTreeStorage() {
    using Initialize = std::uint32_t (*)(
        std::uint32_t, pal4::IEffectSystemTreeStorageRuntime&);
    constexpr std::array<Initialize, 4> initializers{
        pal4::EffectSystemEmbeddedTrees::InitializeSecondStorage,
        pal4::EffectSystemEmbeddedTrees::InitializeThirdStorage,
        pal4::EffectSystemEmbeddedTrees::InitializeFourthStorage,
        pal4::EffectSystemEmbeddedTrees::InitializeFifthStorage};
    constexpr std::array<Kind, 4> kinds{
        Kind::Second, Kind::Third, Kind::Fourth, Kind::Fifth};
    for (std::size_t index = 0; index < initializers.size(); ++index) {
        StorageRuntime runtime;
        runtime.expected_kind = kinds[index];
        runtime.expected_size = 24u;
        const auto head = initializers[index](TreeRuntime::kTree, runtime);
        Check(head == 0x6040u && runtime.nil == 0x6000u &&
              runtime.references == 1u &&
              runtime.words.at(0x6000u) == 0u &&
              runtime.words.at(0x6004u) == 0u &&
              runtime.words.at(0x6008u) == 0u &&
              runtime.words.at(0x6014u) == 1u &&
              runtime.words.at(head) == head &&
              runtime.words.at(head + 4u) == runtime.nil &&
              runtime.words.at(head + 8u) == head &&
              runtime.words.at(head + 20u) == 0u &&
              runtime.words.at(TreeRuntime::kTree + 4u) == head &&
              runtime.words.at(TreeRuntime::kTree + 12u) == 0u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate", "set nil", "set references", "allocate"}),
              "24-byte tree creates nil and head in native order");

        StorageRuntime reused;
        reused.expected_kind = kinds[index];
        reused.expected_size = 24u;
        reused.nil = 0x7000u;
        reused.references = 2u;
        Check(initializers[index](TreeRuntime::kTree, reused) == 0x6040u &&
              reused.nil == 0x7000u && reused.references == 3u &&
              reused.words.at(0x6044u) == 0x7000u &&
              reused.calls == (std::vector<std::string>{
                  "allocate", "set references", "free 24576", "allocate"}),
              "24-byte tree releases a candidate when nil already exists");
    }
}

void TestLaterTreeBranchHelpers() {
    using Delete = std::uint32_t (*)(
        std::uint32_t, std::uint32_t, pal4::IArenaSequenceRuntime&);
    constexpr std::array<Delete, 4> deletions{
        pal4::EffectSystemEmbeddedTrees::DeleteSecondSubtree,
        pal4::EffectSystemEmbeddedTrees::DeleteThirdSubtree,
        pal4::EffectSystemEmbeddedTrees::DeleteFourthSubtree,
        pal4::EffectSystemEmbeddedTrees::DeleteFifthSubtree};
    for (const auto deletion : deletions) {
        InsertRuntime runtime;
        constexpr auto root = 0x5000u;
        constexpr auto left = 0x5100u;
        constexpr auto right = 0x5200u;
        runtime.words[root] = left;
        runtime.words[root + 8u] = right;
        runtime.words[left] = InsertRuntime::kNil;
        runtime.words[left + 8u] = InsertRuntime::kNil;
        runtime.words[right] = InsertRuntime::kNil;
        runtime.words[right + 8u] = InsertRuntime::kNil;
        Check(deletion(root, InsertRuntime::kNil, runtime) ==
                  InsertRuntime::kNil &&
              runtime.frees == (std::vector<std::uint32_t>{
                  right, root, left}),
              "24-byte subtree frees right branch before left spine");
        Check(deletion(InsertRuntime::kNil, InsertRuntime::kNil, runtime) ==
                  InsertRuntime::kNil && runtime.frees.size() == 3u,
              "nil subtree does not free nodes");
    }

    InsertRuntime runtime;
    constexpr auto root = 0x5000u;
    constexpr auto left = 0x5100u;
    runtime.words[root] = left;
    runtime.words[left] = InsertRuntime::kNil;
    runtime.words[0x4000u] = InsertRuntime::kNil;
    runtime.words[0x4004u] = root;
    Check(pal4::EffectSystemEmbeddedTrees::SecondMinimumFromLink(
              0x4000u, InsertRuntime::kNil, runtime) == 0x4000u &&
          pal4::EffectSystemEmbeddedTrees::SecondMinimumFromLink(
              0x4004u, InsertRuntime::kNil, runtime) == left &&
          pal4::EffectSystemEmbeddedTrees::SecondMinimumFromLink(
              root, InsertRuntime::kNil, runtime) == left,
          "second-tree minimum handles nil, root and left spine");
}

void TestFirstBareNodeAllocation() {
    StorageRuntime runtime;
    const auto node = pal4::EffectSystemEmbeddedTrees::CreateFirstNode(
        0x1234u, 1u, runtime);
    Check(node == 0x6000u && runtime.words.at(node + 4u) == 0x1234u &&
          runtime.words.at(node + 24u) == 1u &&
          runtime.words.count(node) == 0u &&
          runtime.words.count(node + 8u) == 0u &&
          runtime.calls == (std::vector<std::string>{"allocate"}),
          "bare first-tree node writes only parent and color");
}

void TestFirstPredecessor() {
    InsertRuntime runtime;
    for (const auto key : {4u, 2u, 6u, 1u, 3u, 5u, 7u}) {
        const auto record = 0x3000u + key * 16u;
        runtime.AddRecord(record, key);
        Check(pal4::EffectSystemEmbeddedTrees::InsertFirst(
                  TreeRuntime::kTree, record, runtime).inserted,
              "first-tree predecessor fixture inserts each key");
    }
    auto iterator = runtime.ReadDword(TreeRuntime::kHead + 8u);
    std::vector<std::uint32_t> seen;
    while (iterator != TreeRuntime::kHead) {
        seen.push_back(runtime.ReadDword(iterator + 12u));
        const auto step = pal4::EffectSystemEmbeddedTrees::PreviousFirst(
            iterator, InsertRuntime::kNil, runtime);
        Check(step.next == iterator,
              "first predecessor updates its iterator");
    }
    Check(seen == (std::vector<std::uint32_t>{7u, 6u, 5u, 4u,
                                              3u, 2u, 1u}),
          "first-tree predecessor traverses reverse order through head");
}

}  // namespace

int main() {
    TestPopulatedTrees();
    TestFallbackAndLastReference();
    TestConstructor();
    TestFirstTreeInsertion();
    TestFirstTreeErasure();
    TestFirstTreeRangeAndSuccessor();
    TestSecondConstructor();
    TestSecondTreeErasure();
    TestSecondTreeRangeAndSuccessor();
    TestFirstTreeStorage();
    TestFirstTreeInsertHelper();
    TestFirstTreeRotationsAndMinimum();
    TestFirstTreeSubtreeDeletion();
    TestLaterTreeStorage();
    TestLaterTreeBranchHelpers();
    TestFirstBareNodeAllocation();
    TestFirstPredecessor();
}
