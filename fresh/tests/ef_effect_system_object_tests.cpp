#include "pal4/ef_effect_system_object.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

using Operation = pal4::EfEffectSystemConstructCall;

enum class EventKind { Scratch, Byte, Word, Allocate, Call,
                       GetAuxiliary, SetAuxiliary, Duplicate };

struct Event {
    EventKind kind;
    std::uint32_t address;
    std::uint32_t value;
    bool operator==(const Event&) const = default;
};

struct Runtime final : pal4::EfEffectSystemConstructRuntimeStub {
    std::vector<Event> events;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;
    std::uint32_t time_allocation = 0x2000u;
    std::uint32_t auxiliary_allocation = 0x2100u;
    std::uint32_t auxiliary_singleton = 0u;
    std::uint32_t allocations = 0u;

    pal4::EfEffectSystemConstructionFlags ScratchFlags() override {
        events.push_back({EventKind::Scratch, 0u, 0u});
        return {.first = 0x5au};
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        events.push_back({EventKind::Byte, address, value});
        bytes[address] = value;
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        events.push_back({EventKind::Word, address, value});
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        events.push_back({EventKind::Allocate, size, 0u});
        Check(size == (allocations == 0u ? 8u : 44u),
              "constructor uses 8-byte then 44-byte allocations");
        return allocations++ == 0u ? time_allocation : auxiliary_allocation;
    }
    std::uint32_t Call(
        Operation operation, std::uint32_t target,
        pal4::EfEffectSystemConstructionFlags& flags) override {
        events.push_back({EventKind::Call, target,
                          static_cast<std::uint32_t>(operation)});
        if (operation == Operation::InitializeTree3) flags.sixth = 0x6bu;
        if (operation == Operation::InitializeTime) {
            Check(target == time_allocation,
                  "time constructor receives allocated this pointer");
            return 0x2020u;
        }
        return target;
    }
    std::uint32_t AuxiliarySingleton() override {
        events.push_back({EventKind::GetAuxiliary, 0u, 0u});
        return auxiliary_singleton;
    }
    void SetAuxiliarySingleton(std::uint32_t object) override {
        events.push_back({EventKind::SetAuxiliary, object, 0u});
        auxiliary_singleton = object;
    }
    void ThrowDuplicateAuxiliary() override {
        events.push_back({EventKind::Duplicate, 0u, 0u});
        throw std::runtime_error("duplicate auxiliary");
    }
};

struct TreeRuntime final : pal4::IEfEffectSystemPrimaryTreeRuntime {
    static constexpr auto kTree = 0x1000u;
    static constexpr auto kHead = 0x2000u;
    static constexpr auto kNode = 0x3000u;
    static constexpr auto kNil = 0x93cde8u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kTree + 4u, kHead}, {kTree + 12u, 0u},
        {kHead, kHead}, {kHead + 4u, kNil},
        {kHead + 8u, kHead}};
    std::uint32_t nil = kNil;
    std::uint32_t references = 2u;
    std::uint32_t next = kHead;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint32_t SharedNil() override { return nil; }
    std::uint32_t ReleaseNilReference() override {
        calls.emplace_back("release reference");
        return --references;
    }
    void ClearSharedNil() override {
        calls.emplace_back("clear nil");
        nil = 0u;
    }
    void DeleteSubtree(std::uint32_t root) override {
        Check(root == kNode, "primary tree deletes its root");
        calls.emplace_back("delete subtree");
    }
    void ReadFirstElement(std::uint32_t tree) override {
        Check(tree == kTree && words.at(kHead) == kHead &&
              words.at(kHead + 4u) == kNil &&
              words.at(kHead + 8u) == kHead &&
              words.at(kTree + 12u) == 0u,
              "first-element helper sees reset tree");
        calls.emplace_back("first element");
    }
    std::uint32_t Next(std::uint32_t node) override {
        Check(node == kNode, "fallback advances node before erasing");
        calls.emplace_back("next");
        return next;
    }
    void EraseNode(std::uint32_t tree,
                   std::uint32_t node) override {
        Check(tree == kTree && node == kNode,
              "fallback erases current node");
        calls.emplace_back("erase node");
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back(address == kHead ? "free head" : "free nil");
        Check(address == kHead || address == kNil,
              "primary cleanup frees only head and final nil");
        return address + 1u;
    }
};

struct DestructRuntime final : pal4::IEfEffectSystemDestructRuntime {
    static constexpr auto kObject = 0x1000u;
    using Tree = pal4::EfEffectSystemOwnedTree;
    using Op = pal4::EfEffectSystemDestructCall;
    std::map<std::uint32_t, std::uint32_t> words{
        {kObject + 144u, 0x5000u}, {kObject + 140u, 0x5100u},
        {kObject + 152u, 0x2000u}, {kObject + 96u, 0x2100u},
        {kObject + 28u, 0x2200u}, {kObject + 12u, 0x2300u},
        {kObject + 36u, 1u}, {kObject + 20u, 1u},
        {0x2200u, 0x3200u}, {0x2300u, 0x3300u}};
    std::uint32_t trailing_references = 1u;
    std::uint32_t active_references = 1u;
    std::vector<std::string> calls;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void DestroyOwned(std::uint32_t object) override {
        calls.push_back("destroy " + std::to_string(object));
    }
    std::uint32_t FirstElement(std::uint32_t tree) override {
        calls.push_back("first " + std::to_string(tree));
        return tree == kObject + 148u ? 0x3000u : 0x3100u;
    }
    void RemoveRange(Tree kind, std::uint32_t tree,
                     std::uint32_t first, std::uint32_t head,
                     std::uint32_t context) override {
        const auto offset = kind == Tree::At148 ? 148u :
            kind == Tree::At92 ? 92u :
            kind == Tree::At24 ? 24u : 8u;
        Check(tree == kObject + offset &&
              head == words.at(tree + 4u) &&
              first == (offset == 148u ? 0x3000u :
                        offset == 92u ? 0x3100u :
                        offset == 24u ? 0x3200u : 0x3300u) &&
              context == (offset >= 92u ? head : 0u),
              "destructor range receives tree, cursor, head and context");
        calls.push_back("remove " + std::to_string(offset));
    }
    std::uint32_t Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        return address + 1u;
    }
    std::uint32_t ReleaseNilReference(Tree kind) override {
        Check(kind == Tree::At148 || kind == Tree::At92,
              "only two trees own explicit nil references here");
        calls.push_back(kind == Tree::At148 ? "release 148" : "release 92");
        return kind == Tree::At148
            ? --trailing_references : --active_references;
    }
    std::uint32_t SharedNil(Tree kind) override {
        return kind == Tree::At148 ? 0x93cde8u : 0x93cdb0u;
    }
    void ClearSharedNil(Tree kind) override {
        calls.push_back(kind == Tree::At148 ? "clear 148" : "clear 92");
    }
    std::uint32_t Call(pal4::EfEffectSystemDestructCall operation,
                       std::uint32_t target) override {
        const auto offset = operation == Op::CleanupState ? 0u :
            operation == Op::DeinitializePool ||
            operation == Op::CleanupPoolObjects ? 84u :
            operation == Op::CleanupTree124 ? 124u :
            operation == Op::CleanupTree108 ? 108u :
            operation == Op::CleanupInstances68 ||
            operation == Op::CleanupTree68 ? 68u :
            operation == Op::DestroyNamedDictionaryTree52 ? 52u :
            operation == Op::DestroyDictionary40 ? 40u :
            operation == Op::CleanupTree24 ? 24u : 8u;
        Check(target == kObject + offset,
              "destructor dispatch uses native member offset");
        calls.push_back("call " +
                        std::to_string(static_cast<int>(operation)));
        return operation == Op::CleanupTree8 ? 0x77u : 0u;
    }
};

struct CleanupRuntime final : pal4::IEfEffectSystemCleanupRuntime {
    static constexpr auto kObject = 0x1000u;
    static constexpr auto kHead = 0x2000u;
    static constexpr auto kFirst = 0x3000u;
    static constexpr auto kSecond = 0x3100u;
    static constexpr auto kNil = 0x93cde8u;
    bool enabled = true;
    std::map<std::uint32_t, std::uint32_t> words{
        {kObject + 152u, kHead},
        {kHead, kFirst}, {kHead + 4u, kFirst},
        {kHead + 8u, kSecond},
        {kFirst, kNil}, {kFirst + 4u, kHead},
        {kFirst + 8u, kSecond}, {kFirst + 12u, 0x7000u},
        {kSecond, kNil}, {kSecond + 4u, kFirst},
        {kSecond + 8u, kNil}, {kSecond + 12u, 0x7100u}};
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kObject, "cleanup checks singleton enabled byte");
        return enabled;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    std::uint32_t SharedNil() override { return kNil; }
    void CleanupEffect(std::uint32_t effect) override {
        calls.push_back("effect " + std::to_string(effect));
    }
    void CleanupInstances(std::uint32_t collection) override {
        Check(collection == kObject + 68u,
              "cleanup uses singleton instance collection");
        calls.emplace_back("instances");
    }
    void SetPoolFlags(std::uint32_t pool,
                      std::uint32_t flags) override {
        Check(pool == kObject + 84u && flags == 32u,
              "cleanup restores native pool flags");
        calls.emplace_back("pool flags");
    }
};

struct EnableRuntime final : pal4::IEfEffectSystemEnableRuntime {
    static constexpr auto kObject = 0x1000u;
    std::vector<std::string> calls;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::uint32_t> words;

    void LogStart() override { calls.emplace_back("log start"); }
    void CleanupState(std::uint32_t object) override {
        Check(object == kObject, "enable cleans prior state first");
        calls.emplace_back("cleanup state");
    }
    void InitializeEvents(std::uint32_t object) override {
        Check(object == kObject, "event setup uses singleton");
        calls.emplace_back("initialize events");
    }
    void RegisterEvents(std::uint32_t object) override {
        Check(object == kObject, "event registration uses singleton");
        calls.emplace_back("register events");
    }
    void InitializeTextureDictionary(std::uint32_t dictionary) override {
        Check(dictionary == kObject + 40u,
              "enable initializes first texture dictionary");
        calls.emplace_back("texture dictionary");
    }
    void SetPoolFlags(std::uint32_t pool,
                      std::uint32_t flags) override {
        Check(pool == kObject + 84u && flags == 32u,
              "enable configures pool flags");
        calls.emplace_back("pool flags");
    }
    void CleanupPoolObjects(std::uint32_t pool) override {
        Check(pool == kObject + 84u,
              "enable cleans object pool after flags");
        calls.emplace_back("pool cleanup");
    }
    void SetTimeAndFlag(double seconds) override {
        Check(seconds == 1200.0, "optional time uses native 1200 seconds");
        calls.emplace_back("set time");
    }
    void WriteByte(std::uint32_t address,
                   std::uint8_t value) override {
        bytes[address] = value;
        calls.push_back("byte " + std::to_string(address));
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void LogComplete() override { calls.emplace_back("log complete"); }
};

struct StateRuntime final : pal4::IEfEffectSystemStateRuntime {
    static constexpr auto kObject = 0x1000u;
    static constexpr auto kFirstHead = 0x2000u;
    static constexpr auto kSecondHead = 0x2100u;
    static constexpr auto kFirstRoot = 0x3000u;
    static constexpr auto kSecondRoot = 0x3100u;
    std::uint8_t active = 1u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kObject + 12u, kFirstHead}, {kObject + 20u, 2u},
        {kObject + 28u, kSecondHead}, {kObject + 36u, 1u},
        {kObject + 144u, 0x4000u},
        {kFirstHead, kFirstRoot}, {kFirstHead + 4u, kFirstRoot},
        {kFirstHead + 8u, kFirstRoot},
        {kSecondHead, kSecondRoot}, {kSecondHead + 4u, kSecondRoot},
        {kSecondHead + 8u, kSecondRoot}};
    std::vector<std::string> calls;

    std::uint8_t ReadByte(std::uint32_t address) override {
        Check(address == kObject, "state cleanup checks active flag");
        calls.emplace_back("read active");
        return active;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        Check(address == kObject && value == 0u,
              "state cleanup clears active flag last");
        active = value;
        calls.emplace_back("clear active");
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write " + std::to_string(address));
    }
    void DeinitializePool(std::uint32_t pool) override {
        Check(pool == kObject + 84u, "deinitialize pool offset");
        calls.emplace_back("pool");
    }
    void CleanupDictionary40(std::uint32_t dictionary) override {
        Check(dictionary == kObject + 40u, "first dictionary offset");
        calls.emplace_back("dictionary 40");
    }
    void CleanupNamedDictionaryTree52(std::uint32_t dictionary) override {
        Check(dictionary == kObject + 52u, "second dictionary offset");
        calls.emplace_back("dictionary 52");
    }
    void CleanupInstances(std::uint32_t collection) override {
        Check(collection == kObject + 68u, "instance collection offset");
        calls.emplace_back("instances");
    }
    void DeleteTreeSubtree(pal4::EfEffectSystemStateTree kind,
                           std::uint32_t root) override {
        const auto first = kind == pal4::EfEffectSystemStateTree::At8;
        Check(root == (first ? kFirstRoot : kSecondRoot),
              "state cleanup deletes tree root");
        calls.emplace_back(first ? "delete tree 8" : "delete tree 24");
    }
    std::uint32_t SharedNil(pal4::EfEffectSystemStateTree kind) override {
        const auto first = kind == pal4::EfEffectSystemStateTree::At8;
        calls.emplace_back(first ? "nil 8" : "nil 24");
        return first ? 0x93cdf4u : 0x93cdf0u;
    }
    void CleanupAuxiliary(std::uint32_t auxiliary) override {
        Check(auxiliary == 0x4000u, "state cleanup forwards auxiliary");
        calls.emplace_back("auxiliary");
    }
};

struct AuxiliaryRuntime final : pal4::IEfEffectSystemAuxiliaryRuntime {
    std::uint32_t singleton = 0x4000u;
    std::vector<std::string> calls;

    void WriteVtable(std::uint32_t object,
                     std::uint32_t vtable) override {
        Check(object == 0x3000u && vtable == 0x84b1c0u,
              "auxiliary initializes its native vtable");
        calls.emplace_back("vtable");
    }
    std::uint32_t CurrentSingleton() override {
        calls.emplace_back("get singleton");
        return singleton;
    }
    void ThrowMissingSingleton() override {
        calls.emplace_back("throw");
        throw std::runtime_error("missing singleton");
    }
    void ClearSingleton() override {
        singleton = 0u;
        calls.emplace_back("clear singleton");
    }
    void Free(std::uint32_t object) override {
        Check(object == 0x3000u,
              "deleting destructor frees auxiliary object");
        calls.emplace_back("free");
    }
};

struct AuxiliaryDestructRuntime final
    : pal4::IEfEffectSystemAuxiliaryDestructRuntime {
    using Tree = pal4::EfEffectSystemAuxiliaryTree;
    static constexpr auto kObject = 0x3000u;
    static constexpr auto kHead28 = 0x5000u;
    static constexpr auto kHead12 = 0x5100u;
    std::map<std::uint32_t, std::uint32_t> words{
        {kObject + 32u, kHead28}, {kObject + 16u, kHead12},
        {kHead28, 0x6000u}, {kHead12, 0x6100u}};
    std::map<Tree, std::uint32_t> references{
        {Tree::At28, 1u}, {Tree::At12, 1u}};
    std::map<Tree, std::uint32_t> nils{
        {Tree::At28, 0x93cdfcu}, {Tree::At12, 0x93ce10u}};
    std::uint32_t singleton = kObject;
    std::vector<std::string> calls;

    void WriteVtable(std::uint32_t object,
                     std::uint32_t vtable) override {
        Check(object == kObject, "auxiliary vtable targets owner");
        Check(vtable == 0x84b1bcu || vtable == 0x84b1c0u,
              "auxiliary destructor uses verified vtable pair");
        calls.emplace_back(vtable == 0x84b1bcu
                               ? "vtable ready" : "vtable base");
    }
    std::uint32_t CurrentSingleton() override {
        calls.emplace_back("get singleton");
        return singleton;
    }
    void ThrowMissingSingleton() override {
        calls.emplace_back("throw");
        throw std::runtime_error("missing singleton");
    }
    void ClearSingleton() override {
        singleton = 0u;
        calls.emplace_back("clear singleton");
    }
    void Free(std::uint32_t pointer) override {
        calls.push_back("free " + std::to_string(pointer));
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words.at(address);
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    void CleanupEffects(std::uint32_t object) override {
        Check(object == kObject, "auxiliary cleanup receives owner");
        calls.emplace_back("effects");
    }
    void RemoveRange(Tree kind, std::uint32_t tree,
                     std::uint32_t first,
                     std::uint32_t head) override {
        const bool second = kind == Tree::At28;
        Check(tree == kObject + (second ? 28u : 12u) &&
              head == (second ? kHead28 : kHead12) &&
              first == (second ? 0x6000u : 0x6100u),
              "auxiliary range receives correct tree and cursors");
        calls.emplace_back(second ? "range 28" : "range 12");
    }
    std::uint32_t ReleaseNilReference(Tree kind) override {
        calls.emplace_back(kind == Tree::At28 ? "release 28" : "release 12");
        return --references[kind];
    }
    std::uint32_t SharedNil(Tree kind) override { return nils.at(kind); }
    void ClearSharedNil(Tree kind) override {
        nils[kind] = 0u;
        calls.emplace_back(kind == Tree::At28 ? "clear 28" : "clear 12");
    }
};

void TestCompleteConstruction() {
    Runtime runtime;
    constexpr auto object = 0x1000u;
    Check(pal4::EfEffectSystemObject::Construct(object, runtime) == object,
          "constructor returns original instance");
    const std::vector<Event> expected{
        {EventKind::Scratch, 0u, 0u},
        {EventKind::Byte, object, 0u},
        {EventKind::Byte, object + 8u, 0x5au},
        {EventKind::Byte, object + 9u, 0x5au},
        {EventKind::Byte, object + 16u, 0u},
        {EventKind::Call, object + 8u,
         static_cast<std::uint32_t>(Operation::InitializeTree12)},
        {EventKind::Call, object + 24u,
         static_cast<std::uint32_t>(Operation::InitializeTree10)},
        {EventKind::Call, object + 40u,
         static_cast<std::uint32_t>(Operation::ConstructTextureDictionary40)},
        {EventKind::Call, object + 52u,
         static_cast<std::uint32_t>(Operation::ConstructNamedDictionaryTree52)},
        {EventKind::Call, object + 68u,
         static_cast<std::uint32_t>(Operation::InitializeTree5)},
        {EventKind::Byte, object + 84u, 0u},
        {EventKind::Byte, object + 85u, 1u},
        {EventKind::Word, object + 88u, 0u},
        {EventKind::Call, object + 92u,
         static_cast<std::uint32_t>(Operation::InitializeTree6)},
        {EventKind::Call, object + 108u,
         static_cast<std::uint32_t>(Operation::InitializeTree2)},
        {EventKind::Call, object + 124u,
         static_cast<std::uint32_t>(Operation::InitializeTree3)},
        {EventKind::Byte, object + 148u, 0x6bu},
        {EventKind::Byte, object + 149u, 0x6bu},
        {EventKind::Byte, object + 156u, 0u},
        {EventKind::Call, object + 148u,
         static_cast<std::uint32_t>(Operation::InitializeTree11)},
        {EventKind::Allocate, 8u, 0u},
        {EventKind::Call, 0x2000u,
         static_cast<std::uint32_t>(Operation::InitializeTime)},
        {EventKind::Word, object + 140u, 0x2020u},
        {EventKind::Allocate, 44u, 0u},
        {EventKind::Word, 0x2100u, 0x84b1c0u},
        {EventKind::GetAuxiliary, 0u, 0u},
        {EventKind::SetAuxiliary, 0x2100u, 0u},
        {EventKind::Byte, 0x2104u, 0u},
        {EventKind::Word, 0x2108u, 0u},
        {EventKind::Call, 0x210cu,
         static_cast<std::uint32_t>(Operation::InitializeTree4)},
        {EventKind::Call, 0x211cu,
         static_cast<std::uint32_t>(Operation::InitializeTree)},
        {EventKind::Word, 0x2100u, 0x84b1bcu},
        {EventKind::Word, object + 144u, 0x2100u},
    };
    Check(runtime.events == expected &&
          runtime.auxiliary_singleton == 0x2100u &&
          runtime.words.at(object + 140u) == 0x2020u &&
          runtime.words.at(object + 144u) == 0x2100u,
          "constructor preserves IDA field and native call order");
}

void TestAllocationBranches() {
    Runtime runtime;
    runtime.time_allocation = 0u;
    runtime.auxiliary_allocation = 0u;
    Check(pal4::EfEffectSystemObject::Construct(0x1000u, runtime) == 0x1000u &&
          runtime.words.at(0x108cu) == 0u &&
          runtime.words.at(0x1090u) == 0u &&
          runtime.auxiliary_singleton == 0u,
          "failed subobject allocations store null pointers");
    for (const auto& event : runtime.events)
        Check(event.kind != EventKind::GetAuxiliary &&
              event.kind != EventKind::SetAuxiliary &&
              event.kind != EventKind::Duplicate &&
              !(event.kind == EventKind::Call &&
                event.value == static_cast<std::uint32_t>(
                    Operation::InitializeTime)),
              "allocation failure skips dependent initialization");

    Runtime duplicate;
    duplicate.auxiliary_singleton = 0x9999u;
    bool threw = false;
    try {
        (void)pal4::EfEffectSystemObject::Construct(0x1000u, duplicate);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && duplicate.auxiliary_singleton == 0x9999u &&
          duplicate.events.back().kind == EventKind::Duplicate &&
          duplicate.words.count(0x1090u) == 0u,
          "duplicate auxiliary throws before publishing owner pointer");
}

void TestPrimaryTreeCleanup() {
    TreeRuntime populated;
    populated.words[TreeRuntime::kTree + 12u] = 1u;
    populated.words[TreeRuntime::kHead] = TreeRuntime::kNode;
    populated.words[TreeRuntime::kHead + 4u] = TreeRuntime::kNode;
    populated.words[TreeRuntime::kHead + 8u] = TreeRuntime::kNode;
    Check(pal4::EfEffectSystemObject::CleanupPrimaryTree(
              TreeRuntime::kTree, populated) == 1u &&
          populated.calls == (std::vector<std::string>{
              "delete subtree", "first element", "free head",
              "release reference"}) &&
          populated.words.at(TreeRuntime::kTree + 4u) == 0u &&
          populated.words.at(TreeRuntime::kTree + 12u) == 0u &&
          populated.nil == TreeRuntime::kNil,
          "populated primary tree resets head and retains referenced nil");

    TreeRuntime fallback;
    fallback.references = 1u;
    fallback.words[TreeRuntime::kHead] = TreeRuntime::kNode;
    Check(pal4::EfEffectSystemObject::CleanupPrimaryTree(
              TreeRuntime::kTree, fallback) ==
              TreeRuntime::kNil + 1u &&
          fallback.calls == (std::vector<std::string>{
              "next", "erase node", "free head", "release reference",
              "clear nil", "free nil"}) &&
          fallback.nil == 0u,
          "zero-count fallback advances before erase and frees last nil");

    TreeRuntime empty;
    empty.references = 1u;
    Check(pal4::EfEffectSystemObject::CleanupPrimaryTree(
              TreeRuntime::kTree, empty) == TreeRuntime::kNil + 1u &&
          empty.calls == (std::vector<std::string>{
              "free head", "release reference", "clear nil", "free nil"}),
          "empty primary tree skips deletion and cursor work");
}

void TestDestructionOrder() {
    DestructRuntime runtime;
    Check(pal4::EfEffectSystemObject::Destruct(
              DestructRuntime::kObject, runtime) == 0x77u &&
          runtime.calls == (std::vector<std::string>{
              "call 0", "destroy 20480", "write 4240",
              "destroy 20736", "write 4236",
              "first 4244", "remove 148", "free 8192",
              "write 4248", "write 4256", "release 148",
              "clear 148", "free 9686504",
              "call 1", "call 2", "call 3", "call 4",
              "first 4188", "remove 92", "free 8448",
              "write 4192", "write 4200", "release 92",
              "clear 92", "free 9686448",
              "call 5", "call 6", "call 7", "call 8",
              "remove 24", "call 9", "remove 8", "call 10"}) &&
          runtime.words.at(DestructRuntime::kObject + 144u) == 0u &&
          runtime.words.at(DestructRuntime::kObject + 140u) == 0u &&
          runtime.words.at(DestructRuntime::kObject + 152u) == 0u &&
          runtime.words.at(DestructRuntime::kObject + 160u) == 0u &&
          runtime.words.at(DestructRuntime::kObject + 96u) == 0u &&
          runtime.words.at(DestructRuntime::kObject + 104u) == 0u,
          "destructor releases owned objects, trees and dictionaries in order");

    DestructRuntime empty;
    empty.words[DestructRuntime::kObject + 144u] = 0u;
    empty.words[DestructRuntime::kObject + 140u] = 0u;
    empty.words[DestructRuntime::kObject + 36u] = 0u;
    empty.words[DestructRuntime::kObject + 20u] = 0u;
    empty.trailing_references = 2u;
    empty.active_references = 2u;
    Check(pal4::EfEffectSystemObject::Destruct(
              DestructRuntime::kObject, empty) == 0x77u &&
          empty.trailing_references == 1u &&
          empty.active_references == 1u &&
          std::find(empty.calls.begin(), empty.calls.end(), "remove 24") ==
              empty.calls.end() &&
          std::find(empty.calls.begin(), empty.calls.end(), "remove 8") ==
              empty.calls.end() &&
          std::find(empty.calls.begin(), empty.calls.end(), "clear 148") ==
              empty.calls.end() &&
          std::find(empty.calls.begin(), empty.calls.end(), "clear 92") ==
              empty.calls.end(),
          "empty branches retain referenced nils and skip optional ranges");
}

void TestRegisteredEffectCleanup() {
    CleanupRuntime runtime;
    pal4::EfEffectSystemObject::CleanupRegisteredEffects(
        CleanupRuntime::kObject, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "effect 28672", "effect 28928", "instances", "pool flags"}),
          "registered effects clean in order before collection and pool");

    CleanupRuntime empty;
    empty.words[CleanupRuntime::kHead] = CleanupRuntime::kHead;
    empty.words[CleanupRuntime::kHead + 4u] = CleanupRuntime::kNil;
    empty.words[CleanupRuntime::kHead + 8u] = CleanupRuntime::kHead;
    pal4::EfEffectSystemObject::CleanupRegisteredEffects(
        CleanupRuntime::kObject, empty);
    Check(empty.calls == (std::vector<std::string>{
              "instances", "pool flags"}),
          "empty registered tree still cleans instances and pool");

    runtime.calls.clear();
    runtime.enabled = false;
    pal4::EfEffectSystemObject::CleanupRegisteredEffects(
        CleanupRuntime::kObject, runtime);
    Check(runtime.calls.empty(),
          "disabled singleton skips registered cleanup");
}

void TestEnable() {
    EnableRuntime runtime;
    Check(pal4::EfEffectSystemObject::Enable(
              EnableRuntime::kObject, true, runtime) == 1u &&
          runtime.calls == (std::vector<std::string>{
              "log start", "cleanup state", "initialize events",
              "register events", "texture dictionary", "pool flags",
              "byte 4181", "pool cleanup", "set time",
              "byte 4097", "byte 4098", "word 4100",
              "word 4260", "byte 4096", "log complete"}) &&
          runtime.bytes.at(EnableRuntime::kObject + 85u) == 1u &&
          runtime.bytes.at(EnableRuntime::kObject + 1u) == 0u &&
          runtime.bytes.at(EnableRuntime::kObject + 2u) == 1u &&
          runtime.bytes.at(EnableRuntime::kObject) == 1u &&
          runtime.words.at(EnableRuntime::kObject + 4u) == 0u &&
          runtime.words.at(EnableRuntime::kObject + 164u) == 0u,
          "enable follows native stages and stores active fields");
    EnableRuntime no_time;
    Check(pal4::EfEffectSystemObject::Enable(
              EnableRuntime::kObject, false, no_time) == 1u &&
          std::find(no_time.calls.begin(), no_time.calls.end(), "set time") ==
              no_time.calls.end() &&
          no_time.bytes.at(EnableRuntime::kObject) == 1u,
          "disabled time option still enables singleton");
}

void TestStateCleanup() {
    StateRuntime runtime;
    pal4::EfEffectSystemObject::CleanupState(StateRuntime::kObject, runtime);
    Check(runtime.calls == (std::vector<std::string>{
              "read active", "pool", "dictionary 40", "dictionary 52",
              "instances", "delete tree 8", "nil 8", "write 8196",
              "write 4116", "write 8192", "write 8200",
              "delete tree 24", "nil 24", "write 8452",
              "write 4132", "write 8448", "write 8456",
              "auxiliary", "write 4260", "write 4100", "clear active"}) &&
          runtime.words.at(StateRuntime::kFirstHead + 4u) == 0x93cdf4u &&
          runtime.words.at(StateRuntime::kSecondHead + 4u) == 0x93cdf0u &&
          runtime.words.at(StateRuntime::kFirstHead) ==
              StateRuntime::kFirstHead &&
          runtime.words.at(StateRuntime::kFirstHead + 8u) ==
              StateRuntime::kFirstHead &&
          runtime.words.at(StateRuntime::kSecondHead) ==
              StateRuntime::kSecondHead &&
          runtime.words.at(StateRuntime::kSecondHead + 8u) ==
              StateRuntime::kSecondHead &&
          runtime.words.at(StateRuntime::kObject + 20u) == 0u &&
          runtime.words.at(StateRuntime::kObject + 36u) == 0u &&
          runtime.words.at(StateRuntime::kObject + 164u) == 0u &&
          runtime.words.at(StateRuntime::kObject + 4u) == 0u &&
          runtime.active == 0u,
          "active cleanup follows native order and resets both trees");

    StateRuntime empty;
    empty.words[StateRuntime::kObject + 20u] = 0u;
    empty.words[StateRuntime::kObject + 36u] = 0u;
    pal4::EfEffectSystemObject::CleanupState(StateRuntime::kObject, empty);
    Check(empty.calls == (std::vector<std::string>{
              "read active", "pool", "dictionary 40", "dictionary 52",
              "instances", "auxiliary", "write 4260", "write 4100",
              "clear active"}) &&
          empty.words.at(StateRuntime::kFirstHead + 4u) ==
              StateRuntime::kFirstRoot &&
          empty.words.at(StateRuntime::kSecondHead + 4u) ==
              StateRuntime::kSecondRoot,
          "zero-count trees skip recursive deletion and header writes");

    StateRuntime inactive;
    inactive.active = 0u;
    pal4::EfEffectSystemObject::CleanupState(StateRuntime::kObject, inactive);
    Check(inactive.calls == (std::vector<std::string>{"read active"}),
          "inactive cleanup has no side effects");
}

void TestAuxiliaryFinalizeSingleton() {
    AuxiliaryRuntime present;
    Check(pal4::EfEffectSystemAuxiliary::FinalizeSingleton(
              0x3000u, present) ==
              0x4000u &&
          present.singleton == 0u &&
          present.calls == (std::vector<std::string>{
              "vtable", "get singleton", "clear singleton"}),
          "auxiliary consumes registered singleton after vtable write");

    AuxiliaryRuntime missing;
    missing.singleton = 0u;
    bool threw = false;
    try {
        (void)pal4::EfEffectSystemAuxiliary::FinalizeSingleton(
            0x3000u, missing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && missing.calls == (std::vector<std::string>{
              "vtable", "get singleton", "throw"}),
          "missing auxiliary singleton throws before clearing global");
}

void TestAuxiliaryDelete() {
    AuxiliaryRuntime retained;
    Check(pal4::EfEffectSystemAuxiliary::Delete(
              0x3000u, 0u, retained) == 0x3000u &&
          retained.calls == (std::vector<std::string>{
              "vtable", "get singleton", "clear singleton"}),
          "deleting destructor can retain object storage");

    AuxiliaryRuntime released;
    Check(pal4::EfEffectSystemAuxiliary::Delete(
              0x3000u, 3u, released) == 0x3000u &&
          released.calls == (std::vector<std::string>{
              "vtable", "get singleton", "clear singleton", "free"}),
          "deleting destructor frees when low flag bit is set");

    AuxiliaryRuntime missing;
    missing.singleton = 0u;
    bool threw = false;
    try {
        (void)pal4::EfEffectSystemAuxiliary::Delete(
            0x3000u, 1u, missing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && missing.calls == (std::vector<std::string>{
              "vtable", "get singleton", "throw"}),
          "missing singleton throws before conditional free");
}

void TestAuxiliaryDestruct() {
    AuxiliaryDestructRuntime runtime;
    Check(pal4::EfEffectSystemAuxiliary::Destruct(
              AuxiliaryDestructRuntime::kObject, runtime) ==
                  AuxiliaryDestructRuntime::kObject &&
          runtime.calls == (std::vector<std::string>{
              "vtable ready", "effects", "range 28", "free 20480",
              "word 12320", "word 12328", "release 28", "clear 28",
              "free " + std::to_string(0x93cdfcu),
              "range 12", "free 20736", "word 12304", "word 12312",
              "release 12", "clear 12",
              "free " + std::to_string(0x93ce10u),
              "vtable base", "get singleton", "clear singleton"}) &&
          runtime.words.at(AuxiliaryDestructRuntime::kObject + 32u) == 0u &&
          runtime.words.at(AuxiliaryDestructRuntime::kObject + 40u) == 0u &&
          runtime.words.at(AuxiliaryDestructRuntime::kObject + 16u) == 0u &&
          runtime.words.at(AuxiliaryDestructRuntime::kObject + 24u) == 0u &&
          runtime.nils.at(AuxiliaryDestructRuntime::Tree::At28) == 0u &&
          runtime.nils.at(AuxiliaryDestructRuntime::Tree::At12) == 0u &&
          runtime.singleton == 0u,
          "auxiliary destructor tears down both trees before singleton");

    AuxiliaryDestructRuntime retained;
    retained.references[AuxiliaryDestructRuntime::Tree::At28] = 2u;
    retained.references[AuxiliaryDestructRuntime::Tree::At12] = 2u;
    (void)pal4::EfEffectSystemAuxiliary::Destruct(
        AuxiliaryDestructRuntime::kObject, retained);
    Check(retained.nils.at(AuxiliaryDestructRuntime::Tree::At28) ==
              0x93cdfcu &&
          retained.nils.at(AuxiliaryDestructRuntime::Tree::At12) ==
              0x93ce10u &&
          std::find(retained.calls.begin(), retained.calls.end(),
                    "clear 28") == retained.calls.end() &&
          std::find(retained.calls.begin(), retained.calls.end(),
                    "clear 12") == retained.calls.end(),
          "remaining nil owners retain both shared sentinels");

    AuxiliaryDestructRuntime missing;
    missing.singleton = 0u;
    bool threw = false;
    try {
        (void)pal4::EfEffectSystemAuxiliary::Destruct(
            AuxiliaryDestructRuntime::kObject, missing);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Check(threw && missing.calls.back() == "throw" &&
          missing.words.at(AuxiliaryDestructRuntime::kObject + 32u) == 0u &&
          missing.words.at(AuxiliaryDestructRuntime::kObject + 16u) == 0u,
          "missing singleton throws after owned trees are cleaned");
}

}  // namespace

int main() {
    TestCompleteConstruction();
    TestAllocationBranches();
    TestPrimaryTreeCleanup();
    TestDestructionOrder();
    TestRegisteredEffectCleanup();
    TestEnable();
    TestStateCleanup();
    TestAuxiliaryFinalizeSingleton();
    TestAuxiliaryDelete();
    TestAuxiliaryDestruct();
}
