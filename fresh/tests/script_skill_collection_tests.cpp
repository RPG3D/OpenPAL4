#include "pal4/script_skill_collection.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptSkillCollectionRuntime {
    std::map<std::uint32_t, std::uint32_t> memory;
    std::vector<std::string> calls;
    std::uint32_t health_updates = 0u;
    std::uint32_t next_allocation = 0xa000u;
    std::uint32_t allocations = 0u;
    std::uint32_t frees = 0u;
    std::vector<std::uint32_t> resolved;

    void InitializeOuterBase(std::uint32_t object) override {
        calls.push_back("outer " + std::to_string(object));
    }
    void InitializeInnerBase(std::uint32_t object) override {
        calls.push_back("inner " + std::to_string(object));
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
        memory[address] = value;
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        calls.push_back("byte " + std::to_string(address) + " " +
                        std::to_string(value));
    }
    std::uint32_t Allocate(std::uint32_t size) override {
        calls.push_back("allocate " + std::to_string(size));
        const auto address = next_allocation;
        next_allocation += size + 0x100u;
        ++allocations;
        return address;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
        ++frees;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read " + std::to_string(address));
        return memory[address];
    }
    std::uint32_t GameDatabase() override {
        calls.push_back("database");
        return 0x1000u;
    }
    void UpdateHealthStatus(std::uint32_t collection) override {
        calls.push_back("health " + std::to_string(collection));
        ++health_updates;
    }
    std::uint32_t ResolveSkillRecord(std::uint32_t skill) override {
        calls.push_back("resolve " + std::to_string(skill));
        resolved.push_back(skill);
        return skill == 0u ? 0x8faa50u : skill;
    }
    std::uint32_t InvokeVirtual(
        std::uint32_t receiver, std::uint32_t offset,
        std::span<const std::uint32_t> arguments) override {
        std::string event = "virtual " + std::to_string(receiver) + " " +
                            std::to_string(offset);
        for (const auto argument : arguments)
            event += " " + std::to_string(argument);
        calls.push_back(event);
        return 37u;
    }
    std::uint32_t SkillSystemInstance() override {
        calls.push_back("system");
        return 0x6000u;
    }
    void ProcessSkillChain(
        std::uint32_t system, std::uint32_t skill,
        std::uint32_t actor, std::uint32_t context,
        std::uint32_t target) override {
        calls.push_back("chain " + std::to_string(system) + " " +
                        std::to_string(skill) + " " +
                        std::to_string(actor) + " " +
                        std::to_string(context) + " " +
                        std::to_string(target));
    }
};

void Initialize() {
    Runtime runtime;
    const auto object = 0x12560000u;
    Check(pal4::ScriptSkillCollection::kSize == 708u &&
              pal4::ScriptSkillCollection::Initialize(object, runtime) ==
                  object &&
              runtime.calls[0] == "outer " + std::to_string(object + 4u) &&
              runtime.calls[1] == "inner " + std::to_string(object + 4u),
          "derived constructor begins with reconstructed base states");
    const std::vector<std::string> expected{
        "byte " + std::to_string(object + 676u) + " 18",
        "word " + std::to_string(object + 680u) + " 0",
        "word " + std::to_string(object + 684u) + " 0",
        "word " + std::to_string(object + 688u) + " 0",
        "byte " + std::to_string(object + 692u) + " 18",
        "word " + std::to_string(object + 696u) + " 0",
        "word " + std::to_string(object + 700u) + " 0",
        "word " + std::to_string(object + 704u) + " 0",
        "word " + std::to_string(object) + " " +
            std::to_string(pal4::ScriptSkillCollection::kVtable),
    };
    Check(runtime.calls.size() >= expected.size() &&
              std::vector<std::string>(runtime.calls.end() - expected.size(),
                                       runtime.calls.end()) == expected &&
              runtime.memory[object] == 0x8430e8u,
          "derived constructor writes both list headers and final vtable");
}

void FadeLookup() {
    Runtime runtime;
    runtime.memory[0x1010u] = 0x2000u;
    runtime.memory[0x2008u] = 0x3000u;
    runtime.memory[0x200cu] = 0x3008u;
    runtime.memory[0x3000u] = 42u;
    runtime.memory[0x3004u] = 2u;
    runtime.memory[0x2060u] = 0x4000u;
    runtime.memory[0x4108u + 8u] = 1u;
    runtime.memory[0x4108u + 12u] = 0xffffffffu;
    Check(pal4::ScriptSkillCollection::HandleFade(
              0x7000u, 42, 0x8000u, runtime) == 1u &&
              runtime.health_updates == 1u &&
              runtime.memory[0x7000u + 696u] == 0xa000u &&
              runtime.memory[0x7000u + 700u] == 0xa008u &&
              runtime.memory[0xa000u] == 0x4108u &&
              runtime.memory[0xa004u] == 0x8000u &&
              runtime.calls.back() == "health 28672",
          "matching GameDB skill reaches the untimed list and callbacks");
    runtime.calls.clear();
    Check(pal4::ScriptSkillCollection::HandleFade(
              0x7000u, 41, 0x8000u, runtime) == 0u &&
              runtime.health_updates == 1u &&
              runtime.calls.front() == "database",
          "missing skill returns zero without processing a list");
}

void TimedListInsertion() {
    Runtime runtime;
    constexpr auto object = 0x7000u;
    constexpr auto skill = 0x5000u;
    runtime.memory[skill + 8u] = 1u;
    runtime.memory[skill + 12u] = 5u;
    runtime.memory[object + 680u] = 0xb000u;
    runtime.memory[object + 684u] = 0xb000u;
    runtime.memory[object + 688u] = 0xb00cu;
    Check(pal4::ScriptSkillCollection::ProcessComplexObjectList(
              object, skill, 0x8000u, runtime) == 1u &&
              runtime.memory[0xb000u] == skill &&
              runtime.memory[0xb004u] == 0x8000u &&
              runtime.memory[0xb008u] == 0u &&
              runtime.memory[object + 684u] == 0xb00cu &&
              runtime.allocations == 0u && runtime.frees == 0u &&
              runtime.health_updates == 1u,
          "timed item uses available 12-byte capacity");
    const auto callback = std::find(runtime.calls.begin(), runtime.calls.end(),
                                    "virtual 28672 300 32768 20480");
    Check(callback != runtime.calls.end() &&
              callback + 1 == runtime.calls.end() - 1 &&
              runtime.calls.back() == "health 28672",
          "slot 300 runs after the skill chain and before health update");

    Runtime growing;
    growing.next_allocation = 0xc000u;
    growing.memory[skill + 8u] = 1u;
    growing.memory[skill + 12u] = 5u;
    growing.memory[skill] = 55u;
    growing.memory[0x9000u] = 77u;
    growing.memory[object + 680u] = 0xb000u;
    growing.memory[object + 684u] = 0xb00cu;
    growing.memory[object + 688u] = 0xb00cu;
    growing.memory[0xb000u] = 0x9000u;
    growing.memory[0xb004u] = 0x9100u;
    growing.memory[0xb008u] = 7u;
    Check(pal4::ScriptSkillCollection::ProcessComplexObjectList(
              object, skill, 0x8000u, growing) == 1u &&
              growing.allocations == 1u && growing.frees == 1u &&
              growing.memory[object + 680u] == 0xc000u &&
              growing.memory[object + 684u] == 0xc018u &&
              growing.memory[object + 688u] == 0xc018u &&
              growing.memory[0xc000u] == 0x9000u &&
              growing.memory[0xc004u] == 0x9100u &&
              growing.memory[0xc008u] == 7u &&
              growing.memory[0xc00cu] == skill &&
              growing.memory[0xc010u] == 0x8000u &&
              growing.memory[0xc014u] == 0u,
          "full timed list grows, copies old entry and appends a new one");
}

void FilterBothLists() {
    Runtime runtime;
    constexpr auto object = 0x7000u;
    runtime.memory[object + 680u] = 0xb000u;
    runtime.memory[object + 684u] = 0xb024u;
    runtime.memory[object + 696u] = 0xc000u;
    runtime.memory[object + 700u] = 0xc010u;
    runtime.memory[0xb000u] = 0x5000u;
    runtime.memory[0xb00cu] = 0x6000u;
    runtime.memory[0xb018u] = 0x5000u;
    runtime.memory[0xc000u] = 0x6000u;
    runtime.memory[0xc008u] = 0x5000u;
    runtime.memory[0x5000u] = 42u;
    runtime.memory[0x6000u] = 77u;
    Check(pal4::ScriptSkillCollection::FilterAndCleanData(
              object, 42u, runtime) == 1u &&
              runtime.memory[0xb000u] == 0u &&
              runtime.memory[0xb00cu] == 0x6000u &&
              runtime.memory[0xb018u] == 0u &&
              runtime.memory[0xc000u] == 0x6000u &&
              runtime.memory[0xc008u] == 0u &&
              runtime.resolved ==
                  std::vector<std::uint32_t>{0x5000u, 0x5000u, 0x5000u},
          "filter clears matching entries in timed list before untimed list");
    std::vector<std::string> callbacks;
    for (const auto& call : runtime.calls)
        if (call.starts_with("virtual ")) callbacks.push_back(call);
    Check(callbacks == std::vector<std::string>{
                           "virtual 28672 304 20480",
                           "virtual 28672 304 20480",
                           "virtual 28672 304 20480",
                           "virtual 28672 276"},
          "each match invokes slot 304 before the final slot 276");

    runtime.calls.clear();
    runtime.resolved.clear();
    Check(pal4::ScriptSkillCollection::FilterAndCleanData(
              object, 99u, runtime) == 1u &&
              runtime.resolved.empty() &&
              runtime.calls.back() == "virtual 28672 276",
          "no-match filter still invokes the final collection update");
}

void DynamicCompactionAndExpiry() {
    Runtime untimed;
    constexpr auto object = 0x7000u;
    untimed.memory[object + 680u] = 0xb018u;
    untimed.memory[object + 684u] = 0xb018u;
    untimed.memory[object + 696u] = 0xc000u;
    untimed.memory[object + 700u] = 0xc018u;
    untimed.memory[0xc000u] = 0x5000u;
    untimed.memory[0xc004u] = 0x1000u;
    untimed.memory[0xc008u] = 0u;
    untimed.memory[0xc010u] = 0x6000u;
    untimed.memory[0xc014u] = 0x2000u;
    Check(pal4::ScriptSkillCollection::ProcessDynamicList(
              object, 5u, untimed) == 0x18u &&
              untimed.memory[object + 700u] == 0xc010u &&
              untimed.memory[0xc008u] == 0x6000u &&
              untimed.memory[0xc00cu] == 0x2000u &&
              untimed.resolved.empty(),
          "empty timed list preserves native low-byte return after 8-byte compaction");

    Runtime timed;
    timed.memory[object + 696u] = 0xc000u;
    timed.memory[object + 700u] = 0xc000u;
    timed.memory[object + 680u] = 0xb000u;
    timed.memory[object + 684u] = 0xb024u;
    timed.memory[0xb000u] = 0u;
    timed.memory[0xb00cu] = 0x5000u;
    timed.memory[0xb010u] = 0x7000u;
    timed.memory[0xb014u] = 4u;
    timed.memory[0xb018u] = 0x6000u;
    timed.memory[0xb01cu] = 0x7100u;
    timed.memory[0xb020u] = 0u;
    timed.memory[0x500cu] = 5u;
    timed.memory[0x600cu] = 10u;
    Check(pal4::ScriptSkillCollection::ProcessDynamicList(
              object, 1u, timed) == 0u &&
              timed.memory[object + 684u] == 0xb018u &&
              timed.memory[0xb000u] == 0u &&
              timed.memory[0xb008u] == 5u &&
              timed.memory[0xb00cu] == 0x6000u &&
              timed.memory[0xb014u] == 1u &&
              timed.resolved == std::vector<std::uint32_t>{0x5000u},
          "12-byte compaction precedes count update and first expiry");
    std::vector<std::string> callbacks;
    for (const auto& call : timed.calls)
        if (call.starts_with("virtual ")) callbacks.push_back(call);
    Check(callbacks == std::vector<std::string>{
                           "virtual 28672 304 20480",
                           "virtual 28672 276"},
          "expiry invokes release and collection update slots in order");

    timed.calls.clear();
    Check(pal4::ScriptSkillCollection::ProcessDynamicList(
              object, 9u, timed) == 37u &&
              timed.memory[object + 684u] == 0xb00cu &&
              timed.memory[0xb000u] == 0u &&
              timed.memory[0xb008u] == 10u &&
              timed.resolved ==
                  std::vector<std::uint32_t>{0x5000u, 0x6000u},
          "previous expiry compacts next call and final expiry forwards slot result");
}

void ExistenceQueries() {
    Runtime runtime;
    constexpr auto object = 0x7000u;
    runtime.memory[object + 696u] = 0xc000u;
    runtime.memory[object + 700u] = 0xc010u;
    runtime.memory[object + 680u] = 0xb000u;
    runtime.memory[object + 684u] = 0xb00cu;
    runtime.memory[0xc000u] = 0x5000u;
    runtime.memory[0xc008u] = 0x6000u;
    runtime.memory[0xb000u] = 0x9000u;
    runtime.memory[0x5000u] = 11u;
    runtime.memory[0x6000u] = 22u;
    runtime.memory[0x9000u] = 33u;
    runtime.memory[0x503cu] = 101u;
    runtime.memory[0x603cu] = 202u;
    runtime.memory[0x903cu] = 303u;
    Check(pal4::ScriptSkillCollection::HasSkillKey(
              object, 22u, runtime) &&
              pal4::ScriptSkillCollection::HasSkillKey(
                  object, 33u, runtime) &&
              !pal4::ScriptSkillCollection::HasSkillKey(
                  object, 44u, runtime) &&
              runtime.resolved.empty(),
          "key query searches 8-byte list before 12-byte list and handles misses");
    Check(pal4::ScriptSkillCollection::HasRecordField60(
              object, 202u, runtime) &&
              runtime.resolved ==
                  std::vector<std::uint32_t>{0x5000u, 0x6000u},
          "record-field query stops at an 8-byte-list match");
    runtime.resolved.clear();
    Check(pal4::ScriptSkillCollection::HasRecordField60(
              object, 303u, runtime) &&
              runtime.resolved == std::vector<std::uint32_t>{
                                      0x5000u, 0x6000u, 0x9000u},
          "record-field query continues into 12-byte list");
    runtime.resolved.clear();
    Check(!pal4::ScriptSkillCollection::HasRecordField60(
              object, 404u, runtime) &&
              runtime.resolved.size() == 3u,
          "record-field query reports a complete miss");

    Runtime empty;
    Check(!pal4::ScriptSkillCollection::HasSkillKey(
              object, 1u, empty) &&
              !pal4::ScriptSkillCollection::HasRecordField60(
                  object, 1u, empty) &&
              empty.resolved.empty(),
          "empty lists have no records to resolve");

    Runtime null_item;
    null_item.memory[object + 696u] = 0xc000u;
    null_item.memory[object + 700u] = 0xc008u;
    null_item.memory[0xc000u] = 0u;
    null_item.memory[0x8faa50u + 60u] = 999u;
    Check(pal4::ScriptSkillCollection::HasSkillKey(
              object, 0xffffffffu, null_item) &&
              pal4::ScriptSkillCollection::HasRecordField60(
                  object, 999u, null_item) &&
              null_item.resolved == std::vector<std::uint32_t>{0u},
          "null list slot uses minus-one key and the global fallback record");
}

void BatchAndDirectSkill() {
    Runtime runtime;
    constexpr auto object = 0x7000u;
    constexpr auto target = 0x8000u;
    runtime.memory[object + 696u] = 0xc000u;
    runtime.memory[object + 700u] = 0xc010u;
    runtime.memory[object + 680u] = 0xb000u;
    runtime.memory[object + 684u] = 0xb00cu;
    runtime.memory[0xc000u] = 0x5000u;
    runtime.memory[0xc004u] = 0x7100u;
    runtime.memory[0xc008u] = 0x6000u;
    runtime.memory[0xc00cu] = 0x7200u;
    runtime.memory[0xb000u] = 0x9000u;
    runtime.memory[0xb004u] = 0x7300u;
    runtime.memory[0x5008u] = 1u;
    runtime.memory[0x6008u] = 2u;
    runtime.memory[0x9008u] = 1u;
    Check(pal4::ScriptSkillCollection::ProcessSkillsBatch(
              object, 1u, target, runtime) == 0xb00cu,
          "batch returns the timed-list end pointer");
    std::vector<std::string> events;
    for (const auto& call : runtime.calls)
        if (call.starts_with("virtual ") || call.starts_with("chain "))
            events.push_back(call);
    Check(events == std::vector<std::string>{
                        "virtual 28672 280 20480 1 28928 32768",
                        "virtual 32768 308 28928 20480 1",
                        "chain 24576 20480 28672 28928 32768",
                        "virtual 28672 284 20480 1 28928 32768",
                        "virtual 32768 312 28928 20480 1",
                        "virtual 28672 280 36864 1 29440 32768",
                        "virtual 32768 308 29440 36864 1",
                        "chain 24576 36864 28672 29440 32768",
                        "virtual 28672 284 36864 1 29440 32768",
                        "virtual 32768 312 29440 36864 1"},
          "batch handles untimed list first and skips mismatched skill IDs");

    Runtime direct;
    direct.memory[0x1010u] = 0x2000u;
    direct.memory[0x2008u] = 0x3000u;
    direct.memory[0x200cu] = 0x3008u;
    direct.memory[0x3000u] = 42u;
    direct.memory[0x3004u] = 2u;
    direct.memory[0x2060u] = 0x4000u;
    direct.memory[0x4108u + 8u] = 99u;
    Check(pal4::ScriptSkillCollection::TriggerSkillForTarget(
              object, 42, target, direct) &&
              direct.health_updates == 1u,
          "direct skill lookup dispatches even when stored skill ID differs");
    events.clear();
    for (const auto& call : direct.calls)
        if (call.starts_with("virtual ") || call.starts_with("chain ") ||
            call.starts_with("health ")) events.push_back(call);
    Check(events == std::vector<std::string>{
                        "virtual 28672 280 16648 2 28672 32768",
                        "virtual 32768 308 28672 16648 2",
                        "chain 24576 16648 28672 28672 32768",
                        "virtual 28672 284 16648 2 28672 32768",
                        "virtual 32768 312 28672 16648 2",
                        "health 32768"},
          "direct skill executes actor/target callbacks and health update in order");
    direct.calls.clear();
    Check(!pal4::ScriptSkillCollection::TriggerSkillForTarget(
              object, 41, target, direct) &&
              direct.health_updates == 1u,
          "missing direct skill returns false without callbacks");
}

void CleanupAndReset() {
    Runtime runtime;
    constexpr auto object = 0x7000u;
    runtime.memory[object + 680u] = 0xb000u;
    runtime.memory[object + 684u] = 0xb018u;
    runtime.memory[object + 696u] = 0xc000u;
    runtime.memory[object + 700u] = 0xc010u;
    runtime.memory[0xb000u] = 0x5000u;
    runtime.memory[0xb00cu] = 0x6000u;
    runtime.memory[0xc000u] = 0x9000u;
    runtime.memory[0xc008u] = 0xa000u;
    Check(pal4::ScriptSkillCollection::CleanupObjectLists(
              object, runtime) == 37u &&
              runtime.memory[object + 684u] == 0xb000u &&
              runtime.memory[object + 700u] == 0xc000u &&
              runtime.resolved == std::vector<std::uint32_t>{
                                      0x5000u, 0x6000u, 0x9000u, 0xa000u},
          "cleanup repeatedly removes first timed item then first untimed item");
    std::vector<std::string> callbacks;
    for (const auto& call : runtime.calls)
        if (call.starts_with("virtual ")) callbacks.push_back(call);
    Check(callbacks == std::vector<std::string>{
                           "virtual 28672 304 20480",
                           "virtual 28672 304 24576",
                           "virtual 28672 304 36864",
                           "virtual 28672 304 40960"},
          "cleanup invokes slot 304 in removal order");

    Runtime timed_only;
    timed_only.memory[object + 680u] = 0xb000u;
    timed_only.memory[object + 684u] = 0xb00cu;
    timed_only.memory[0xb000u] = 0x5000u;
    timed_only.memory[object + 696u] = 0xc008u;
    timed_only.memory[object + 700u] = 0xc008u;
    Check(pal4::ScriptSkillCollection::CleanupObjectLists(
              object, timed_only) == 0xc008u &&
              timed_only.resolved == std::vector<std::uint32_t>{0x5000u},
          "empty untimed list returns its end pointer after timed cleanup");

    runtime.calls.clear();
    Check(pal4::ScriptSkillCollection::ResetObjectProperties(
              object, runtime) == 37u,
          "property reset forwards the final virtual result");
    callbacks.clear();
    for (const auto& call : runtime.calls)
        if (call.starts_with("virtual ")) callbacks.push_back(call);
    Check(callbacks == std::vector<std::string>{
                           "virtual 28672 140 0",
                           "virtual 28672 148 0",
                           "virtual 28672 156 0",
                           "virtual 28672 164 0",
                           "virtual 28672 180 0",
                           "virtual 28672 188 0",
                           "virtual 28672 196 0",
                           "virtual 28672 204 0"},
          "property reset invokes all eight slots in IDA order");
}

void FadeUpdateBranches() {
    constexpr auto object = 0x7000u;
    constexpr auto skill = 0x5000u;
    Runtime untimed;
    untimed.memory[skill] = 42u;
    untimed.memory[skill + 8u] = 2u;
    untimed.memory[skill + 12u] = 0xffffffffu;
    untimed.memory[object + 696u] = 0xc000u;
    untimed.memory[object + 700u] = 0xc010u;
    untimed.memory[0xc000u] = 0x6000u;
    untimed.memory[0xc008u] = 0x9000u;
    untimed.memory[0x6000u] = 42u;
    untimed.memory[0x9000u] = 55u;
    Check(pal4::ScriptSkillCollection::ProcessObjectFadeAndListUpdate(
              object, skill, untimed) == 0x10u &&
              untimed.memory[0xc000u] == 0u &&
              untimed.memory[0xc008u] == 0x9000u,
          "unset skill without field60 clears same-key 8-byte entry");

    Runtime timed;
    timed.memory[skill] = 42u;
    timed.memory[skill + 8u] = 2u;
    timed.memory[skill + 12u] = 5u;
    timed.memory[object + 680u] = 0xb000u;
    timed.memory[object + 684u] = 0xb018u;
    timed.memory[0xb000u] = 0x6000u;
    timed.memory[0xb00cu] = 0x9000u;
    timed.memory[0x6000u] = 42u;
    timed.memory[0x9000u] = 55u;
    Check(pal4::ScriptSkillCollection::ProcessObjectFadeAndListUpdate(
              object, skill, timed) == 55u &&
              timed.memory[0xb000u] == 0u &&
              timed.memory[0xb00cu] == 0x9000u &&
              std::find(timed.calls.begin(), timed.calls.end(),
                        "virtual 28672 296 20480") != timed.calls.end(),
          "timed skill clears key match and returns final nonmatch getter byte");
    timed.memory[skill + 8u] = 1u;
    Check(pal4::ScriptSkillCollection::ProcessObjectFadeAndListUpdate(
              object, skill, timed) == 37u &&
              timed.calls.back() == "virtual 28672 276",
          "skill type one invokes final slot 276 regardless of list match");
}

void ConditionalCleanup() {
    constexpr auto object = 0x7000u;
    Runtime empty;
    empty.memory[object + 696u] = 0xc008u;
    empty.memory[object + 700u] = 0xc008u;
    Check(pal4::ScriptSkillCollection::ConditionalListCleanup(
              object, 9u, true, empty) == 8u &&
              empty.resolved.empty(),
          "empty selected list returns low byte of its end pointer");

    Runtime untimed;
    untimed.memory[object + 696u] = 0xc000u;
    untimed.memory[object + 700u] = 0xc010u;
    untimed.memory[0xc000u] = 0x6000u;
    untimed.memory[0xc008u] = 0x9000u;
    untimed.memory[0x6000u + 60u] = 9u;
    untimed.memory[0x6000u + 8u] = 1u;
    untimed.memory[0x9000u + 60u] = 5u;
    Check(pal4::ScriptSkillCollection::ConditionalListCleanup(
              object, 9u, true, untimed) == 37u &&
              untimed.memory[0xc000u] == 0u &&
              untimed.memory[0xc008u] == 0x9000u &&
              untimed.resolved == std::vector<std::uint32_t>{
                                      0x6000u, 0x6000u, 0x9000u},
          "field60 match resolves twice and schedules slot 276 for type one");
    std::vector<std::string> callbacks;
    for (const auto& call : untimed.calls)
        if (call.starts_with("virtual ")) callbacks.push_back(call);
    Check(callbacks == std::vector<std::string>{
                           "virtual 28672 296 24576",
                           "virtual 28672 276"},
          "conditional cleanup invokes slot 296 before final slot 276");

    Runtime timed;
    timed.memory[object + 680u] = 0xb000u;
    timed.memory[object + 684u] = 0xb00cu;
    timed.memory[0xb000u] = 0x5000u;
    timed.memory[0x5000u + 60u] = 9u;
    timed.memory[0x5000u + 8u] = 2u;
    Check(pal4::ScriptSkillCollection::ConditionalListCleanup(
              object, 9u, false, timed) == 0u &&
              timed.memory[0xb000u] == 0u &&
              std::find(timed.calls.begin(), timed.calls.end(),
                        "virtual 28672 276") == timed.calls.end(),
          "timed field60 match without type one skips final update");

    Runtime outer;
    outer.memory[0x5000u + 60u] = 9u;
    outer.memory[0x5000u + 12u] = 0xffffffffu;
    outer.memory[0x5000u + 8u] = 1u;
    outer.memory[object + 696u] = 0xc000u;
    outer.memory[object + 700u] = 0xc008u;
    outer.memory[0xc000u] = 0x6000u;
    outer.memory[0x6000u + 60u] = 9u;
    outer.memory[0x6000u + 8u] = 1u;
    Check(pal4::ScriptSkillCollection::ProcessObjectFadeAndListUpdate(
              object, 0x5000u, outer) == 37u &&
              outer.memory[0xc000u] == 0u,
          "nonzero field60 routes through conditional cleanup");
    callbacks.clear();
    for (const auto& call : outer.calls)
        if (call.starts_with("virtual ")) callbacks.push_back(call);
    Check(callbacks == std::vector<std::string>{
                           "virtual 28672 296 24576",
                           "virtual 28672 276",
                           "virtual 28672 276"},
          "conditional cleanup and outer type-one update each invoke slot 276");
}

}  // namespace

int main() {
    try {
        Initialize();
        FadeLookup();
        TimedListInsertion();
        FilterBothLists();
        DynamicCompactionAndExpiry();
        ExistenceQueries();
        BatchAndDirectSkill();
        CleanupAndReset();
        FadeUpdateBranches();
        ConditionalCleanup();
        std::cout << "script skill collection tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
