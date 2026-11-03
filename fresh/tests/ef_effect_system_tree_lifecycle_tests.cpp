#include "pal4/ef_effect_system_tree_lifecycle.h"
#include "pal4/arena_sequence_resources.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

using Kind = pal4::EfEffectSystemTreeKind;
using Lifecycle = pal4::EfEffectSystemTreeLifecycle;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

struct Runtime final : pal4::IEfEffectSystemInstanceTreeRuntime {
    static constexpr auto kTree = 0x1000u;
    static constexpr auto kHead = 0x2000u;
    static constexpr auto kRoot = 0x3000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<Kind, std::uint32_t> nils;
    std::map<Kind, std::uint32_t> references;
    std::vector<std::string> calls;
    std::uint32_t next_allocation = kHead;
    std::uint32_t next_node = kHead;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        Check(bytes == 20u || bytes == 24u || bytes == 36u,
              "tree allocation uses native node size");
        calls.push_back("allocate " + std::to_string(bytes));
        const auto address = next_allocation;
        next_allocation += 0x100u;
        return address;
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t SharedNil(Kind kind) override { return nils[kind]; }
    void SetSharedNil(Kind kind, std::uint32_t value) override {
        nils[kind] = value;
        calls.emplace_back(value == 0u ? "clear nil" : "set nil");
    }
    std::uint32_t SharedNilReferences(Kind kind) override {
        return references[kind];
    }
    void SetSharedNilReferences(Kind kind,
                                std::uint32_t value) override {
        references[kind] = value;
        calls.push_back("references " + std::to_string(value));
    }
    void DeleteSubtree(Kind kind, std::uint32_t root) override {
        Check(root == kRoot && kind != Kind::First,
              "cleanup delegates the root to the matching tree");
        calls.emplace_back("delete subtree");
    }
    std::uint32_t Next(Kind kind, std::uint32_t node) override {
        Check(kind != Kind::First && node == kRoot,
              "fallback advances before deleting current node");
        calls.emplace_back("next");
        return next_node;
    }
    void EraseNode(Kind kind, std::uint32_t tree,
                   std::uint32_t node) override {
        Check(kind != Kind::First && tree == kTree && node == kRoot,
              "fallback erases the node from its own tree");
        calls.emplace_back("erase");
    }
    void ReadFirstElement(std::uint32_t tree) override {
        Check(tree == kTree && words.at(kTree + 12u) == 0u &&
              words.at(kHead) == kHead,
              "direct cleanup reads first element after head reset");
        calls.emplace_back("first element");
    }
    void CleanupInstances(std::uint32_t tree) override {
        Check(tree == kTree, "instance cleanup receives ArenaSequence tree");
        calls.emplace_back("cleanup instances");
    }
};

struct WrapperRuntime final : pal4::IEfEffectSystemTreeWrapperRuntime {
    using WrapperKind = pal4::EfEffectSystemTreeWrapperKind;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::string> calls;

    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void InitializeStorage(WrapperKind kind,
                           std::uint32_t tree) override {
        Check(tree == Runtime::kTree,
              "wrapper forwards member tree to storage initializer");
        calls.push_back("storage " +
                        std::to_string(static_cast<unsigned>(kind)));
    }
};

struct RangeRuntime final : pal4::IEfEffectSystemRangeRuntime {
    using RangeKind = pal4::EfEffectSystemRangeKind;
    static constexpr auto kTree = 0x1000u;
    static constexpr auto kHead = 0x2000u;
    static constexpr auto kRoot = 0x3000u;
    static constexpr auto kLeft = 0x3100u;
    static constexpr auto kResult = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t nil = 0x93cdb0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void DeleteSubtree(RangeKind, std::uint32_t root) override {
        Check(root == nil, "range deletion releases right branch first");
        calls.emplace_back("delete right");
    }
    void ReleaseText(std::uint32_t record) override {
        calls.push_back("text " + std::to_string(record));
    }
    std::uint32_t Free(std::uint32_t node) override {
        calls.push_back("free " + std::to_string(node));
        return node + 1u;
    }
    std::uint32_t Next(RangeKind, std::uint32_t node) override {
        calls.push_back("next " + std::to_string(node));
        return node == kLeft ? kRoot : kHead;
    }
    void EraseNode(RangeKind kind, std::uint32_t tree,
                   std::uint32_t node) override {
        Check(tree == kTree, "partial range erases from original tree");
        const auto label = kind == RangeKind::EffectObject ? "erase object " :
            kind == RangeKind::Key ? "erase key " :
            kind == RangeKind::Primary ? "erase primary " :
            kind == RangeKind::Secondary ? "erase secondary " :
            kind == RangeKind::AuxiliaryFirst ? "erase auxiliary first " :
                                                 "erase auxiliary second ";
        calls.push_back(std::string(label) + std::to_string(node));
    }
};

struct TextTreeRuntime final : pal4::IArenaSequenceRuntime {
    static constexpr auto kTree = 0x1000u;
    static constexpr auto kHead = 0x2000u;
    static constexpr auto kNode = 0x3000u;
    static constexpr auto kResult = 0x4000u;
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> frees;

    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes.at(address);
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t) override {
        Check(false, "node erasure does not allocate");
        return 0u;
    }
    std::uint32_t Free(std::uint32_t pointer) override {
        frees.push_back(pointer);
        return pointer + 1u;
    }
    std::uint32_t Call(pal4::ArenaSequenceCall,
                       std::initializer_list<std::uint32_t>) override {
        Check(false, "text tree cleanup needs no native call");
        return 0u;
    }
};

std::uint32_t Initialize(Kind kind, Runtime& runtime) {
    switch (kind) {
    case Kind::First:
        return Lifecycle::InitializeFirst(Runtime::kTree, 0x5au, 0x6bu,
                                          runtime);
    case Kind::EffectObject:
        return Lifecycle::InitializeEffectObject(Runtime::kTree,
                                                  0x5au, 0x6bu, runtime);
    case Kind::NameCount:
        return Lifecycle::InitializeNameCount(Runtime::kTree,
                                               0x5au, 0x6bu, runtime);
    case Kind::ArenaSequence:
        break;
    }
    return 0u;
}

std::uint32_t Cleanup(Kind kind, Runtime& runtime) {
    switch (kind) {
    case Kind::ArenaSequence:
        return Lifecycle::CleanupArenaSequence(Runtime::kTree, runtime);
    case Kind::EffectObject:
        return Lifecycle::CleanupEffectObject(Runtime::kTree, runtime);
    case Kind::NameCount:
        return Lifecycle::CleanupNameCount(Runtime::kTree, runtime);
    case Kind::First:
        break;
    }
    return 0u;
}

std::uint32_t CleanupPrimarySecondary(Kind kind, bool direct,
                                      Runtime& runtime) {
    if (kind == Kind::Primary)
        return direct
            ? Lifecycle::CleanupPrimaryDirect(Runtime::kTree, runtime)
            : Lifecycle::CleanupPrimaryOnUnwind(Runtime::kTree, runtime);
    return direct
        ? Lifecycle::CleanupSecondaryDirect(Runtime::kTree, runtime)
        : Lifecycle::CleanupSecondaryOnUnwind(Runtime::kTree, runtime);
}

void TestInitializers() {
    for (const auto kind : {Kind::First, Kind::EffectObject,
                            Kind::NameCount}) {
        Runtime runtime;
        const auto size = kind == Kind::First ? 24u : 36u;
        const auto color = kind == Kind::First ? 20u : 32u;
        Check(Initialize(kind, runtime) == Runtime::kTree &&
              runtime.calls == (std::vector<std::string>{
                  "allocate " + std::to_string(size), "set nil",
                  "references 1", "allocate " + std::to_string(size)}) &&
              runtime.bytes.at(Runtime::kTree) == 0x5au &&
              runtime.bytes.at(Runtime::kTree + 1u) == 0x6bu &&
              runtime.bytes.at(Runtime::kTree + 8u) ==
                  (kind == Kind::EffectObject ? 1u : 0u) &&
              runtime.nils.at(kind) == 0x2000u &&
              runtime.references.at(kind) == 1u &&
              runtime.words.at(0x2000u + color) == 1u &&
              runtime.words.at(0x2100u + color) == 0u &&
              runtime.words.at(0x2100u + 4u) == 0x2000u &&
              runtime.words.at(0x2100u) == 0x2100u &&
              runtime.words.at(0x2100u + 8u) == 0x2100u &&
              runtime.words.at(Runtime::kTree + 12u) == 0u,
              "first owner creates correct nil and head layout");

        runtime.calls.clear();
        Check(Initialize(kind, runtime) == Runtime::kTree &&
              runtime.calls == (std::vector<std::string>{
                  "allocate " + std::to_string(size), "references 2",
                  "free 8704", "allocate " + std::to_string(size)}) &&
              runtime.nils.at(kind) == 0x2000u &&
              runtime.references.at(kind) == 2u &&
              runtime.words.at(Runtime::kTree + 4u) == 0x2300u,
              "later owner discards candidate nil and reuses shared nil");
    }
}

Runtime CleanupFixture(Kind kind, std::uint32_t count,
                       std::uint32_t references) {
    Runtime runtime;
    runtime.nils[kind] = kind == Kind::ArenaSequence ? 0x8f605cu :
        kind == Kind::EffectObject ? 0x93cdb0u :
        kind == Kind::NameCount ? 0x93cdacu :
        kind == Kind::Primary ? 0x93cdf4u : 0x93cdf0u;
    if (kind == Kind::AuxiliaryFirst) runtime.nils[kind] = 0x93ce10u;
    if (kind == Kind::AuxiliarySecond) runtime.nils[kind] = 0x93cdfcu;
    runtime.references[kind] = references;
    runtime.words[Runtime::kTree + 4u] = Runtime::kHead;
    runtime.words[Runtime::kTree + 12u] = count;
    runtime.words[Runtime::kHead] = count ? Runtime::kRoot : Runtime::kHead;
    runtime.words[Runtime::kHead + 4u] = Runtime::kRoot;
    runtime.words[Runtime::kHead + 8u] = Runtime::kRoot;
    return runtime;
}

void TestCleanup() {
    for (const auto kind : {Kind::ArenaSequence, Kind::EffectObject,
                            Kind::NameCount}) {
        auto populated = CleanupFixture(kind, 1u, 2u);
        Check(Cleanup(kind, populated) == 1u &&
              populated.calls == (std::vector<std::string>{
                  "delete subtree", "free 8192", "references 1"}) &&
              populated.words.at(Runtime::kTree + 4u) == 0u &&
              populated.words.at(Runtime::kTree + 12u) == 0u &&
              populated.nils.at(kind) != 0u,
              "nonempty cleanup keeps a referenced nil");

        auto fallback = CleanupFixture(kind, 0u, 1u);
        fallback.words[Runtime::kHead] = Runtime::kRoot;
        const auto final_nil = fallback.nils.at(kind);
        const auto result = Cleanup(kind, fallback);
        const std::vector<std::string> expected{
                  "next", "erase", "free 8192", "references 0",
                  "clear nil", "free " + std::to_string(
                      kind == Kind::ArenaSequence ? 0x8f605cu :
                      kind == Kind::EffectObject ? 0x93cdb0u :
                      0x93cdacu)};
        Check(result == final_nil + 1u && fallback.calls == expected,
              "zero-count fallback erases before freeing final nil");

        auto empty = CleanupFixture(kind, 0u, 1u);
        empty.words[Runtime::kHead] = Runtime::kHead;
        (void)Cleanup(kind, empty);
        Check(empty.calls.size() == 4u &&
              empty.calls.front() == "free 8192" &&
              empty.nils.at(kind) == 0u,
              "empty tree skips both removal paths");
    }
}

void TestPrimarySecondaryCleanup() {
    for (const auto kind : {Kind::Primary, Kind::Secondary}) {
        for (const bool direct : {false, true}) {
            auto populated = CleanupFixture(kind, 1u, 2u);
            Check(CleanupPrimarySecondary(kind, direct, populated) == 1u &&
                  populated.calls == (direct
                      ? std::vector<std::string>{
                          "delete subtree", "first element", "free 8192",
                          "references 1"}
                      : std::vector<std::string>{
                          "delete subtree", "free 8192", "references 1"}) &&
                  populated.words.at(Runtime::kHead + 4u) ==
                      (kind == Kind::Primary ? 0x93cdf4u : 0x93cdf0u) &&
                  populated.words.at(Runtime::kTree + 4u) == 0u &&
                  populated.words.at(Runtime::kTree + 12u) == 0u,
                  "both cleanup variants reset populated tree and retain nil");

            auto fallback = CleanupFixture(kind, 0u, 1u);
            fallback.words[Runtime::kHead] = Runtime::kRoot;
            const auto nil = fallback.nils.at(kind);
            Check(CleanupPrimarySecondary(kind, direct, fallback) == nil + 1u &&
                  fallback.calls == (std::vector<std::string>{
                      "next", "erase", "free 8192", "references 0",
                      "clear nil", "free " + std::to_string(nil)}) &&
                  fallback.nils.at(kind) == 0u,
                  "zero-count fallback advances before erasing and frees nil");
        }
        auto plain = CleanupFixture(kind, 1u, 1u);
        const auto nil = plain.nils.at(kind);
        const auto result = kind == Kind::Primary
            ? Lifecycle::CleanupPrimaryPlain(Runtime::kTree, plain)
            : Lifecycle::CleanupSecondaryPlain(Runtime::kTree, plain);
        Check(result == nil + 1u &&
              plain.calls == (std::vector<std::string>{
                  "delete subtree", "free 8192", "references 0",
                  "clear nil", "free " + std::to_string(nil)}),
              "plain cleanup releases last nil without first-element read");
    }
}

void TestEraseRanges() {
    for (const bool object_record : {true, false}) {
        RangeRuntime full;
        full.nil = object_record ? 0x93cdb0u : 0x93cde8u;
        full.words = {
            {RangeRuntime::kTree + 4u, RangeRuntime::kHead},
            {RangeRuntime::kTree + 12u, 2u},
            {RangeRuntime::kHead, RangeRuntime::kLeft},
            {RangeRuntime::kHead + 4u, RangeRuntime::kRoot},
            {RangeRuntime::kHead + 8u, RangeRuntime::kRoot},
            {RangeRuntime::kRoot, RangeRuntime::kLeft},
            {RangeRuntime::kRoot + 8u, full.nil},
            {RangeRuntime::kLeft, full.nil},
            {RangeRuntime::kLeft + 8u, full.nil},
        };
        const auto erase = [&](RangeRuntime& runtime,
                               std::uint32_t first,
                               std::uint32_t last) {
            return object_record
                ? Lifecycle::EraseEffectObjectRange(
                    RangeRuntime::kTree, RangeRuntime::kResult,
                    first, last, runtime.nil, runtime)
                : Lifecycle::EraseKeyRange(
                    RangeRuntime::kTree, RangeRuntime::kResult,
                    first, last, runtime.nil, runtime);
        };
        const auto expected_full = object_record
            ? std::vector<std::string>{
                "delete right", "text 12300", "free 12288",
                "delete right", "text 12556", "free 12544",
                "write 8196", "write 4108", "write 8192",
                "write 8200", "write 16384"}
            : std::vector<std::string>{
                "delete right", "free 12288", "delete right",
                "free 12544", "write 8196", "write 4108",
                "write 8192", "write 8200", "write 16384"};
        Check(erase(full, RangeRuntime::kLeft, RangeRuntime::kHead) ==
                  RangeRuntime::kResult &&
              full.calls == expected_full &&
              full.words.at(RangeRuntime::kHead + 4u) == full.nil &&
              full.words.at(RangeRuntime::kTree + 12u) == 0u &&
              full.words.at(RangeRuntime::kResult) == RangeRuntime::kHead,
              "full range frees right-first nodes and resets head");

        RangeRuntime partial;
        partial.nil = full.nil;
        partial.words[RangeRuntime::kTree + 4u] = RangeRuntime::kHead;
        partial.words[RangeRuntime::kTree + 12u] = 2u;
        partial.words[RangeRuntime::kHead] = RangeRuntime::kLeft;
        Check(erase(partial, RangeRuntime::kLeft,
                    RangeRuntime::kRoot) == RangeRuntime::kResult &&
              partial.calls == (std::vector<std::string>{
                  "next 12544",
                  std::string(object_record ? "erase object " :
                                            "erase key ") + "12544",
                  "write 16384"}) &&
              partial.words.at(RangeRuntime::kResult) == RangeRuntime::kRoot,
              "partial range advances before erasing its first node");

        RangeRuntime empty;
        empty.nil = full.nil;
        empty.words[RangeRuntime::kTree + 4u] = RangeRuntime::kHead;
        empty.words[RangeRuntime::kTree + 12u] = 0u;
        Check(erase(empty, RangeRuntime::kHead,
                    RangeRuntime::kHead) == RangeRuntime::kResult &&
              empty.calls == (std::vector<std::string>{"write 16384"}) &&
              empty.words.at(RangeRuntime::kResult) == RangeRuntime::kHead,
              "empty range only returns its original cursor");
    }
}

void TestConstructorWrappers() {
    using WrapperKind = pal4::EfEffectSystemTreeWrapperKind;
    for (const auto kind : {WrapperKind::AuxiliaryFirst,
                            WrapperKind::Instances, WrapperKind::Active,
                            WrapperKind::Secondary}) {
        WrapperRuntime runtime;
        const auto invoke = [&]() {
            switch (kind) {
            case WrapperKind::AuxiliaryFirst:
                return Lifecycle::InitializeAuxiliaryFirst(
                    Runtime::kTree, 0x12u, 0x34u, 0x56u, runtime);
            case WrapperKind::Instances:
                return Lifecycle::InitializeInstances(
                    Runtime::kTree, 0x12u, 0x34u, 0x56u, runtime);
            case WrapperKind::Active:
                return Lifecycle::InitializeActive(
                    Runtime::kTree, 0x12u, 0x34u, 0x56u, runtime);
            case WrapperKind::Secondary:
                return Lifecycle::InitializeSecondary(
                    Runtime::kTree, 0x12u, 0x34u, 0x56u, runtime);
            }
            return 0u;
        };
        Check(invoke() == Runtime::kTree &&
              runtime.calls == (std::vector<std::string>{
                  "byte 4096", "byte 4104", "byte 4097",
                  "storage " +
                      std::to_string(static_cast<unsigned>(kind))}) &&
              runtime.bytes.at(Runtime::kTree) == 0x12u &&
              runtime.bytes.at(Runtime::kTree + 1u) == 0x34u &&
              runtime.bytes.at(Runtime::kTree + 8u) == 0x56u,
              "wrapper writes flags and forwards to matching tree storage");
    }
}

void TestKeyAdvance() {
    Runtime runtime;
    constexpr auto nil = 0x93cde8u;
    runtime.words[0x3000u + 8u] = 0x3100u;
    runtime.words[0x3100u] = nil;
    auto iterator = 0x3000u;
    Check(Lifecycle::AdvanceKey(iterator, nil, runtime) == nil &&
          iterator == 0x3100u,
          "key successor descends to leftmost right child");

    runtime.words[0x3100u + 8u] = nil;
    runtime.words[0x3100u + 4u] = 0x3000u;
    runtime.words[0x3000u + 8u] = nil;
    Check(Lifecycle::AdvanceKey(iterator, nil, runtime) == 0x3000u &&
          iterator == 0x3000u,
          "key successor ascends to its parent");
}

void TestStorageInitializers() {
    for (const auto kind : {Kind::AuxiliaryStorage, Kind::KeyStorage,
                            Kind::ArenaSequence, Kind::EffectObject,
                            Kind::Primary, Kind::Secondary}) {
        Runtime runtime;
        const auto size = kind == Kind::AuxiliaryStorage ||
            kind == Kind::KeyStorage ? 20u : 36u;
        const auto color = size == 20u ? 16u : 32u;
        const auto initialize = [&]() {
            switch (kind) {
            case Kind::AuxiliaryStorage:
                return Lifecycle::InitializeAuxiliaryStorage(
                    Runtime::kTree, runtime);
            case Kind::KeyStorage:
                return Lifecycle::InitializeKeyStorage(
                    Runtime::kTree, runtime);
            case Kind::ArenaSequence:
                return Lifecycle::InitializeArenaSequenceStorage(
                    Runtime::kTree, runtime);
            case Kind::EffectObject:
                return Lifecycle::InitializeEffectObjectStorage(
                    Runtime::kTree, runtime);
            case Kind::Primary:
                return Lifecycle::InitializePrimaryStorage(
                    Runtime::kTree, runtime);
            case Kind::Secondary:
                return Lifecycle::InitializeSecondaryStorage(
                    Runtime::kTree, runtime);
            default:
                return 0u;
            }
        };
        Check(initialize() == 0x2100u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate " + std::to_string(size), "set nil",
                  "references 1", "allocate " + std::to_string(size)}) &&
              runtime.words.at(0x2000u + color) == 1u &&
              runtime.words.at(0x2000u + 4u) == 0u &&
              runtime.words.at(0x2100u + color) == 0u &&
              runtime.words.at(0x2100u + 4u) == 0x2000u &&
              runtime.words.at(Runtime::kTree + 4u) == 0x2100u &&
              runtime.words.at(Runtime::kTree + 12u) == 0u &&
              runtime.bytes.empty(),
              "storage initializer creates native nil and head layout");

        runtime.calls.clear();
        Check(initialize() == 0x2300u &&
              runtime.calls == (std::vector<std::string>{
                  "allocate " + std::to_string(size), "references 2",
                  "free 8704", "allocate " + std::to_string(size)}) &&
              runtime.nils.at(kind) == 0x2000u &&
              runtime.references.at(kind) == 2u &&
              runtime.words.at(Runtime::kTree + 4u) == 0x2300u,
              "storage initializer reuses shared nil for another owner");
    }
}

void TestMinimumFromLink() {
    for (const auto nil : {0x93cde8u, 0x93cdb0u}) {
        Runtime runtime;
        constexpr auto link = 0x4000u;
        runtime.words[link] = nil;
        const auto minimum = [&]() {
            return nil == 0x93cde8u
                ? Lifecycle::MinimumKeyFromLink(link, nil, runtime)
                : Lifecycle::MinimumEffectObjectFromLink(link, nil, runtime);
        };
        Check(minimum() == link,
              "empty child link returns original link address");
        runtime.words[link] = 0x4100u;
        runtime.words[0x4100u] = 0x4200u;
        runtime.words[0x4200u] = nil;
        Check(minimum() == 0x4200u,
              "minimum helper follows left links to last non-nil node");
    }
}

void TestTextTreeCursorCapture() {
    for (const bool primary : {true, false}) {
        Runtime runtime;
        const auto nil = primary ? 0x93cdf4u : 0x93cdf0u;
        constexpr auto result = 0x5000u;
        const auto advance = [&](std::uint32_t& cursor) {
            return primary
                ? Lifecycle::AdvancePrimaryAndCapture(
                    cursor, result, nil, runtime)
                : Lifecycle::AdvanceSecondaryAndCapture(
                    cursor, result, nil, runtime);
        };
        runtime.words[0x3000u + 8u] = 0x3100u;
        runtime.words[0x3100u] = nil;
        auto cursor = 0x3000u;
        Check(advance(cursor) == result && cursor == 0x3100u &&
              runtime.words.at(result) == 0x3000u,
              "text-tree cursor captures old node on right-subtree descent");
        runtime.words[0x3100u + 8u] = nil;
        runtime.words[0x3100u + 4u] = 0x3000u;
        runtime.words[0x3000u + 8u] = nil;
        Check(advance(cursor) == result && cursor == 0x3000u &&
              runtime.words.at(result) == 0x3100u,
              "text-tree cursor captures old node on parent ascent");
    }
}

void TestTextTreeMinimaAndSuccessors() {
    for (const bool primary : {true, false}) {
        Runtime runtime;
        const auto nil = primary ? 0x93cdf4u : 0x93cdf0u;
        constexpr auto link = 0x4000u;
        runtime.words[link] = nil;
        const auto minimum = [&]() {
            return primary
                ? Lifecycle::MinimumPrimaryFromLink(link, nil, runtime)
                : Lifecycle::MinimumSecondaryFromLink(link, nil, runtime);
        };
        Check(minimum() == link,
              "empty text-tree link returns link address");
        runtime.words[link] = 0x4100u;
        runtime.words[0x4100u] = 0x4200u;
        runtime.words[0x4200u] = nil;
        Check(minimum() == 0x4200u,
              "text-tree minimum descends through left links");

        const auto advance = [&](std::uint32_t& iterator) {
            return primary
                ? Lifecycle::AdvancePrimary(iterator, nil, runtime)
                : Lifecycle::AdvanceSecondary(iterator, nil, runtime);
        };
        runtime.words[0x3000u + 8u] = 0x3100u;
        runtime.words[0x3100u] = nil;
        auto iterator = 0x3000u;
        Check(advance(iterator) == nil && iterator == 0x3100u,
              "text-tree successor descends to right-subtree minimum");
        runtime.words[0x3100u + 8u] = nil;
        runtime.words[0x3100u + 4u] = 0x3000u;
        runtime.words[0x3000u + 8u] = nil;
        Check(advance(iterator) == 0x3000u && iterator == 0x3000u,
              "text-tree successor ascends to parent");
    }
}

void TestTextTreeNodeAndSubtreeCleanup() {
    for (const bool primary : {true, false}) {
        const auto nil = primary ? 0x93cdf4u : 0x93cdf0u;
        TextTreeRuntime runtime;
        runtime.words = {
            {TextTreeRuntime::kTree + 4u, TextTreeRuntime::kHead},
            {TextTreeRuntime::kTree + 12u, 1u},
            {TextTreeRuntime::kHead, TextTreeRuntime::kNode},
            {TextTreeRuntime::kHead + 4u, TextTreeRuntime::kNode},
            {TextTreeRuntime::kHead + 8u, TextTreeRuntime::kNode},
            {TextTreeRuntime::kNode, nil},
            {TextTreeRuntime::kNode + 4u, TextTreeRuntime::kHead},
            {TextTreeRuntime::kNode + 8u, nil},
            {TextTreeRuntime::kNode + 16u, 0x5001u},
            {TextTreeRuntime::kNode + 32u, 1u},
            {nil, nil}, {nil + 4u, 0u},
            {nil + 8u, nil}, {nil + 32u, 1u},
        };
        runtime.bytes[0x5000u] = 2u;
        const auto erase = primary
            ? Lifecycle::ErasePrimaryNode(
                TextTreeRuntime::kTree, TextTreeRuntime::kResult,
                TextTreeRuntime::kNode, nil, runtime)
            : Lifecycle::EraseSecondaryNode(
                TextTreeRuntime::kTree, TextTreeRuntime::kResult,
                TextTreeRuntime::kNode, nil, runtime);
        Check(erase == TextTreeRuntime::kResult &&
              runtime.words.at(TextTreeRuntime::kResult) ==
                  TextTreeRuntime::kHead &&
              runtime.words.at(TextTreeRuntime::kTree + 12u) == 0u &&
              runtime.words.at(TextTreeRuntime::kHead + 4u) == nil &&
              runtime.words.at(TextTreeRuntime::kNode + 16u) == 0u &&
              runtime.bytes.at(0x5000u) == 1u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{TextTreeRuntime::kNode}),
              "text-tree node erase restores head and decrements text owner");

        TextTreeRuntime subtree;
        constexpr auto left = 0x3100u;
        constexpr auto right = 0x3200u;
        subtree.words[TextTreeRuntime::kNode] = left;
        subtree.words[TextTreeRuntime::kNode + 8u] = right;
        subtree.words[left] = nil;
        subtree.words[left + 8u] = nil;
        subtree.words[right] = nil;
        subtree.words[right + 8u] = nil;
        for (const auto node : {TextTreeRuntime::kNode, left, right})
            subtree.words[node + 16u] = 0u;
        const auto deleted = primary
            ? Lifecycle::DeletePrimarySubtree(
                TextTreeRuntime::kNode, nil, subtree)
            : Lifecycle::DeleteSecondarySubtree(
                TextTreeRuntime::kNode, nil, subtree);
        Check(deleted == nil &&
              subtree.frees == (std::vector<std::uint32_t>{
                  right, TextTreeRuntime::kNode, left}),
              "text-tree subtree frees right branch before left spine");
    }
}

void TestAuxiliaryNodeAndSubtreeCleanup() {
    for (const bool first_tree : {true, false}) {
        const auto nil = first_tree ? 0x93ce10u : 0x93cdfcu;
        const auto color = first_tree ? 16u : 20u;
        TextTreeRuntime runtime;
        runtime.words = {
            {TextTreeRuntime::kTree + 4u, TextTreeRuntime::kHead},
            {TextTreeRuntime::kTree + 12u, 1u},
            {TextTreeRuntime::kHead, TextTreeRuntime::kNode},
            {TextTreeRuntime::kHead + 4u, TextTreeRuntime::kNode},
            {TextTreeRuntime::kHead + 8u, TextTreeRuntime::kNode},
            {TextTreeRuntime::kNode, nil},
            {TextTreeRuntime::kNode + 4u, TextTreeRuntime::kHead},
            {TextTreeRuntime::kNode + 8u, nil},
            {TextTreeRuntime::kNode + color, 1u},
            {nil, nil}, {nil + 4u, 0u},
            {nil + 8u, nil}, {nil + color, 1u},
        };
        const auto erased = first_tree
            ? Lifecycle::EraseAuxiliaryFirstNode(
                TextTreeRuntime::kTree, TextTreeRuntime::kResult,
                TextTreeRuntime::kNode, nil, runtime)
            : Lifecycle::EraseAuxiliarySecondNode(
                TextTreeRuntime::kTree, TextTreeRuntime::kResult,
                TextTreeRuntime::kNode, nil, runtime);
        Check(erased == TextTreeRuntime::kResult &&
              runtime.words.at(TextTreeRuntime::kResult) ==
                  TextTreeRuntime::kHead &&
              runtime.words.at(TextTreeRuntime::kTree + 12u) == 0u &&
              runtime.words.at(TextTreeRuntime::kHead + 4u) == nil &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{TextTreeRuntime::kNode}),
              "auxiliary node erasure repairs its head without text calls");
    }

    TextTreeRuntime subtree;
    constexpr auto nil = 0x93ce10u;
    constexpr auto left = 0x3100u;
    constexpr auto right = 0x3200u;
    subtree.words[TextTreeRuntime::kNode] = left;
    subtree.words[TextTreeRuntime::kNode + 8u] = right;
    subtree.words[left] = nil;
    subtree.words[left + 8u] = nil;
    subtree.words[right] = nil;
    subtree.words[right + 8u] = nil;
    Check(Lifecycle::DeleteAuxiliaryFirstSubtree(
              TextTreeRuntime::kNode, nil, subtree) == nil &&
          subtree.frees == (std::vector<std::uint32_t>{
              right, TextTreeRuntime::kNode, left}),
          "auxiliary subtree releases right branch before left spine");
    subtree.frees.clear();
    Check(Lifecycle::DeleteAuxiliaryFirstSubtree(
              nil, nil, subtree) == nil && subtree.frees.empty(),
          "nil auxiliary branch is retained");
}

void TestAuxiliarySecondSubtree() {
    TextTreeRuntime runtime;
    constexpr auto nil = 0x93cdfcu;
    constexpr auto left = 0x3100u;
    constexpr auto right = 0x3200u;
    runtime.words = {
        {TextTreeRuntime::kNode, left},
        {TextTreeRuntime::kNode + 8u, right},
        {left, nil}, {left + 8u, nil},
        {right, nil}, {right + 8u, nil},
    };
    Check(Lifecycle::DeleteAuxiliarySecondSubtree(
              TextTreeRuntime::kNode, nil, runtime) == nil &&
          runtime.frees == (std::vector<std::uint32_t>{
              right, TextTreeRuntime::kNode, left}),
          "second auxiliary subtree frees right branch before left spine");
    runtime.frees.clear();
    Check(Lifecycle::DeleteAuxiliarySecondSubtree(nil, nil, runtime) == nil &&
          runtime.frees.empty(), "second auxiliary nil is retained");
}

void TestAuxiliaryCursorsAndCleanup() {
    for (const bool first : {true, false}) {
        const auto kind = first ? Kind::AuxiliaryFirst : Kind::AuxiliarySecond;
        const auto nil = first ? 0x93ce10u : 0x93cdfcu;
        Runtime cursor;
        cursor.words[0x4000u] = nil;
        Check((first
                   ? Lifecycle::MinimumAuxiliaryFirstFromLink(
                         0x4000u, nil, cursor)
                   : Lifecycle::MinimumAuxiliarySecondFromLink(
                         0x4000u, nil, cursor)) == 0x4000u,
              "empty auxiliary link returns its own address");
        cursor.words[0x4000u] = 0x3000u;
        cursor.words[0x3000u] = 0x3100u;
        cursor.words[0x3100u] = nil;
        Check((first
                   ? Lifecycle::MinimumAuxiliaryFirstFromLink(
                         0x4000u, nil, cursor)
                   : Lifecycle::MinimumAuxiliarySecondFromLink(
                         0x4000u, nil, cursor)) == 0x3100u,
              "auxiliary minimum follows both left links");

        cursor.words[0x3000u + 8u] = 0x3200u;
        cursor.words[0x3200u] = nil;
        auto iterator = 0x3000u;
        const auto advance = [&]() {
            return first
                ? Lifecycle::AdvanceAuxiliaryFirst(iterator, nil, cursor)
                : Lifecycle::AdvanceAuxiliarySecond(iterator, nil, cursor);
        };
        Check(advance() == nil && iterator == 0x3200u,
              "auxiliary successor descends into right child");
        cursor.words[0x3200u + 8u] = nil;
        cursor.words[0x3200u + 4u] = 0x3000u;
        cursor.words[0x3000u + 8u] = nil;
        Check(advance() == 0x3000u && iterator == 0x3000u,
              "auxiliary successor ascends through parent");

        auto retained = CleanupFixture(kind, 1u, 2u);
        const auto clean = [&](Runtime& runtime) {
            return first
                ? Lifecycle::CleanupAuxiliaryFirst(Runtime::kTree, runtime)
                : Lifecycle::CleanupAuxiliarySecond(Runtime::kTree, runtime);
        };
        Check(clean(retained) == 1u &&
              retained.calls == (std::vector<std::string>{
                  "delete subtree", "first element", "free 8192",
                  "references 1"}) &&
              retained.words.at(Runtime::kHead + 4u) == nil &&
              retained.nils.at(kind) == nil,
              "populated auxiliary cleanup resets head and retains shared nil");

        auto final = CleanupFixture(kind, 0u, 1u);
        final.words[Runtime::kHead] = Runtime::kRoot;
        Check(clean(final) == nil + 1u &&
              final.calls == (std::vector<std::string>{
                  "next", "erase", "free 8192", "references 0",
                  "clear nil", "free " + std::to_string(nil)}) &&
              final.nils.at(kind) == 0u,
              "zero-count auxiliary fallback releases final shared nil");
    }
}

void TestArenaSequenceDirectCleanup() {
    auto instances = CleanupFixture(Kind::ArenaSequence, 1u, 2u);
    Check(Lifecycle::CleanupArenaSequenceWithInstances(
              Runtime::kTree, instances) == 1u &&
          instances.calls == (std::vector<std::string>{
              "cleanup instances", "delete subtree", "first element",
              "free 8192", "references 1"}) &&
          instances.words.at(Runtime::kHead + 4u) == 0x8f605cu,
          "instance cleanup runs before direct ArenaSequence teardown");

    auto direct = CleanupFixture(Kind::ArenaSequence, 0u, 1u);
    direct.words[Runtime::kHead] = Runtime::kRoot;
    Check(Lifecycle::CleanupArenaSequenceDirect(
              Runtime::kTree, direct) == 0x8f605du &&
          direct.calls == (std::vector<std::string>{
              "next", "erase", "free 8192", "references 0",
              "clear nil", "free " + std::to_string(0x8f605cu)}),
          "direct ArenaSequence fallback releases the last nil reference");
}

void TestEffectAndNameDirectCleanup() {
    for (const auto kind : {Kind::EffectObject, Kind::NameCount}) {
        const auto clean = [&](Runtime& runtime) {
            return kind == Kind::EffectObject
                ? Lifecycle::CleanupEffectObjectDirect(Runtime::kTree,
                                                       runtime)
                : Lifecycle::CleanupNameCountDirect(Runtime::kTree,
                                                    runtime);
        };
        auto populated = CleanupFixture(kind, 1u, 2u);
        Check(clean(populated) == 1u &&
              populated.calls == (std::vector<std::string>{
                  "delete subtree", "first element", "free 8192",
                  "references 1"}) &&
              populated.words.at(Runtime::kHead + 4u) ==
                  populated.nils.at(kind),
              "direct effect/name cleanup refreshes populated head");

        auto fallback = CleanupFixture(kind, 0u, 1u);
        fallback.words[Runtime::kHead] = Runtime::kRoot;
        const auto nil = fallback.nils.at(kind);
        Check(clean(fallback) == nil + 1u &&
              fallback.calls == (std::vector<std::string>{
                  "next", "erase", "free 8192", "references 0",
                  "clear nil", "free " + std::to_string(nil)}),
              "direct effect/name fallback releases final shared nil");
    }
}

void TestPrimarySecondaryRanges() {
    for (const bool primary : {true, false}) {
        const auto nil = primary ? 0x93cdf4u : 0x93cdf0u;
        const auto erase = [&](RangeRuntime& runtime,
                               std::uint32_t first,
                               std::uint32_t last) {
            return primary
                ? Lifecycle::ErasePrimaryRange(
                    RangeRuntime::kTree, RangeRuntime::kResult,
                    first, last, nil, runtime)
                : Lifecycle::EraseSecondaryRange(
                    RangeRuntime::kTree, RangeRuntime::kResult,
                    first, last, nil, runtime);
        };
        RangeRuntime full;
        full.nil = nil;
        full.words = {
            {RangeRuntime::kTree + 4u, RangeRuntime::kHead},
            {RangeRuntime::kTree + 12u, 1u},
            {RangeRuntime::kHead, RangeRuntime::kRoot},
            {RangeRuntime::kHead + 4u, RangeRuntime::kRoot},
            {RangeRuntime::kHead + 8u, RangeRuntime::kRoot},
            {RangeRuntime::kRoot, nil},
            {RangeRuntime::kRoot + 8u, nil},
        };
        Check(erase(full, RangeRuntime::kRoot, RangeRuntime::kHead) ==
                  RangeRuntime::kResult &&
              full.calls == (std::vector<std::string>{
                  "delete right", "text 12300", "free 12288",
                  "write 8196", "write 4108", "write 8192",
                  "write 8200", "write 16384"}) &&
              full.words.at(RangeRuntime::kHead + 4u) == nil &&
              full.words.at(RangeRuntime::kTree + 12u) == 0u,
              "primary and secondary full ranges release text and reset head");

        RangeRuntime partial;
        partial.nil = nil;
        partial.words[RangeRuntime::kTree + 4u] = RangeRuntime::kHead;
        partial.words[RangeRuntime::kTree + 12u] = 2u;
        partial.words[RangeRuntime::kHead] = RangeRuntime::kLeft;
        Check(erase(partial, RangeRuntime::kRoot,
                    RangeRuntime::kHead) == RangeRuntime::kResult &&
              partial.calls == (std::vector<std::string>{
                  "next 12288",
                  std::string(primary ? "erase primary " :
                                        "erase secondary ") + "12288",
                  "write 16384"}) &&
              partial.words.at(RangeRuntime::kResult) == RangeRuntime::kHead,
              "partial range advances before erasing matching tree node");
    }
}

void TestAuxiliaryRanges() {
    for (const bool first_tree : {true, false}) {
        const auto nil = first_tree ? 0x93ce10u : 0x93cdfcu;
        const auto erase = [&](RangeRuntime& runtime,
                               std::uint32_t first,
                               std::uint32_t last) {
            return first_tree
                ? Lifecycle::EraseAuxiliaryFirstRange(
                    RangeRuntime::kTree, RangeRuntime::kResult,
                    first, last, nil, runtime)
                : Lifecycle::EraseAuxiliarySecondRange(
                    RangeRuntime::kTree, RangeRuntime::kResult,
                    first, last, nil, runtime);
        };
        RangeRuntime full;
        full.nil = nil;
        full.words = {
            {RangeRuntime::kTree + 4u, RangeRuntime::kHead},
            {RangeRuntime::kTree + 12u, 1u},
            {RangeRuntime::kHead, RangeRuntime::kRoot},
            {RangeRuntime::kHead + 4u, RangeRuntime::kRoot},
            {RangeRuntime::kHead + 8u, RangeRuntime::kRoot},
            {RangeRuntime::kRoot, nil},
            {RangeRuntime::kRoot + 8u, nil},
        };
        Check(erase(full, RangeRuntime::kRoot, RangeRuntime::kHead) ==
                  RangeRuntime::kResult &&
              full.calls == (std::vector<std::string>{
                  "delete right", "free 12288", "write 8196",
                  "write 4108", "write 8192", "write 8200",
                  "write 16384"}) &&
              full.words.at(RangeRuntime::kHead + 4u) == nil &&
              full.words.at(RangeRuntime::kTree + 12u) == 0u,
              "auxiliary full range frees nodes without text callbacks");

        RangeRuntime partial;
        partial.nil = nil;
        partial.words[RangeRuntime::kTree + 4u] = RangeRuntime::kHead;
        partial.words[RangeRuntime::kTree + 12u] = 2u;
        partial.words[RangeRuntime::kHead] = RangeRuntime::kLeft;
        Check(erase(partial, RangeRuntime::kRoot,
                    RangeRuntime::kHead) == RangeRuntime::kResult &&
              partial.calls == (std::vector<std::string>{
                  "next 12288",
                  std::string(first_tree ? "erase auxiliary first " :
                                           "erase auxiliary second ") +
                      "12288",
                  "write 16384"}) &&
              partial.words.at(RangeRuntime::kResult) == RangeRuntime::kHead,
              "auxiliary partial range advances before erasing one node");
    }
}

}  // namespace

int main() {
    TestInitializers();
    TestCleanup();
    TestPrimarySecondaryCleanup();
    TestConstructorWrappers();
    TestEraseRanges();
    TestKeyAdvance();
    TestStorageInitializers();
    TestPrimarySecondaryRanges();
    TestAuxiliaryRanges();
    TestMinimumFromLink();
    TestTextTreeCursorCapture();
    TestTextTreeMinimaAndSuccessors();
    TestTextTreeNodeAndSubtreeCleanup();
    TestAuxiliaryNodeAndSubtreeCleanup();
    TestAuxiliarySecondSubtree();
    TestAuxiliaryCursorsAndCleanup();
    TestArenaSequenceDirectCleanup();
    TestEffectAndNameDirectCleanup();
}
