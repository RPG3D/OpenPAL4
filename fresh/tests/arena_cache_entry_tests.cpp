#include "pal4/arena_cache_entry.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using Entry = pal4::ArenaCacheEntry;

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IArenaCacheEntryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::map<std::uint32_t, std::string> names;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> allocation_sizes;
    std::deque<std::uint32_t> input_words;
    std::map<std::uint32_t, std::uint32_t> child_names;
    std::map<std::uint32_t, std::uint32_t> live_objects;
    std::function<void(Runtime&)> iterate_objects;
    std::function<void(Runtime&)> iterate_monsters;
    std::uint32_t next_allocation = 0xa000u;
    std::vector<std::string> events;
    struct Invocation {
        pal4::ArenaCacheEntryCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const Invocation&) const = default;
    };
    std::vector<Invocation> calls;
    std::uint32_t comparisons = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(const std::uint32_t address,
                   const std::uint32_t value) override {
        events.push_back("write:" + std::to_string(address));
        words[address] = value;
    }
    std::uint8_t ReadByte(const std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(const std::uint32_t address,
                   const std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Free(const std::uint32_t address) override {
        events.push_back("free:" + std::to_string(address));
        frees.push_back(address);
        return address + 0x10000u;
    }
    std::uint32_t Allocate(const std::uint32_t size) override {
        allocation_sizes.push_back(size);
        const auto result = next_allocation;
        next_allocation += 0x100u;
        return result;
    }
    bool NamesEqualIgnoreCase(const std::uint32_t left,
                              const std::uint32_t right) override {
        ++comparisons;
        auto first = names[left];
        auto second = names[right];
        for (auto& ch : first)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        for (auto& ch : second)
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return first == second;
    }
    std::uint32_t Call(const pal4::ArenaCacheEntryCall operation,
                       const std::initializer_list<std::uint32_t> arguments)
        override {
        calls.push_back({operation, arguments});
        switch (operation) {
        case pal4::ArenaCacheEntryCall::ReadDataWord: {
            Check(!input_words.empty(), "input data must contain a word");
            const auto value = input_words.front();
            input_words.pop_front();
            return value;
        }
        case pal4::ArenaCacheEntryCall::FindStorageObject:
            return *arguments.begin() + 0x1000u;
        case pal4::ArenaCacheEntryCall::DeserializeObject:
            return *arguments.begin() + 0x2000u;
        case pal4::ArenaCacheEntryCall::AllocateText: {
            const auto record = *arguments.begin();
            words[record + 4u] = next_allocation;
            next_allocation += 0x100u;
            return 1u;
        }
        case pal4::ArenaCacheEntryCall::IterateGameObjects:
            if (iterate_objects) iterate_objects(*this);
            return 0u;
        case pal4::ArenaCacheEntryCall::IterateMonsters:
            if (iterate_monsters) iterate_monsters(*this);
            return 0u;
        case pal4::ArenaCacheEntryCall::CaptureMinimap:
            return 0x9000u;
        case pal4::ArenaCacheEntryCall::SnapshotGameObject:
            return *arguments.begin() + 0x100u;
        case pal4::ArenaCacheEntryCall::SnapshotMonster:
            return *arguments.begin() + 0x200u;
        case pal4::ArenaCacheEntryCall::GetChildName:
            return child_names[*arguments.begin()];
        case pal4::ArenaCacheEntryCall::FindGameObjectByName:
        case pal4::ArenaCacheEntryCall::FindMonsterByName:
            return live_objects[*arguments.begin()];
        case pal4::ArenaCacheEntryCall::DeleteChild:
            return *arguments.begin() + 0x100u;
        case pal4::ArenaCacheEntryCall::GetChildValue:
            return *arguments.begin() + 1u;
        case pal4::ArenaCacheEntryCall::ProcessChild:
            return *arguments.begin() + 2u;
        default:
            return 0u;
        }
    }
};

void Construction() {
    Runtime runtime;
    constexpr std::uint32_t entry = 0x7e006000u;
    Check(Entry::Initialize(entry, runtime) == entry &&
              runtime.words[entry] == Entry::kVtable &&
              runtime.words[entry + 68u] == 0u,
          "constructor writes vtable and final word");
    for (const auto offset : {4u, 20u, 36u, 52u}) {
        Check(runtime.bytes[entry + offset] == 0x7eu &&
                  runtime.words[entry + offset + 4u] == 0u &&
                  runtime.words[entry + offset + 8u] == 0u &&
                  runtime.words[entry + offset + 12u] == 0u,
              "four records preserve saved-ECX high byte and clear three words");
    }
    Check(runtime.frees.empty(), "construction does not release memory");
}

void CleanupCountedAndOwned() {
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    runtime.words[entry + 56u] = 0x1000u;
    runtime.words[entry + 40u] = 0x2000u;
    runtime.words[entry + 24u] = 0x3001u;
    runtime.words[entry + 8u] = 0x4001u;
    runtime.bytes[0x3000u] = 2u;
    runtime.bytes[0x4000u] = 0xffu;
    Check(Entry::Cleanup(entry, runtime) == 0x14000u &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0x1000u, 0x2000u, 0x4000u}) &&
              runtime.bytes[0x3000u] == 1u,
          "owned blocks free first; counted blocks decrement or free");
    Check(runtime.words[entry] == Entry::kVtable,
          "cleanup restores entry vtable");
    for (const auto offset : {8u, 24u, 40u, 56u})
        Check(runtime.words[entry + offset] == 0u &&
                  runtime.words[entry + offset + 4u] == 0u &&
                  runtime.words[entry + offset + 8u] == 0u,
              "cleanup clears all four pointer triples");
    Check(runtime.events[0] == "write:24576" &&
              runtime.events[1] == "free:4096" &&
              runtime.events[5] == "free:8192",
          "vtable write and direct free order match IDA");
}

void DeletionFlagsAndCounts() {
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    runtime.words[entry + 8u] = 0x5001u;
    runtime.bytes[0x5000u] = 1u;
    Check(Entry::Destroy(entry, 0u, runtime) == entry &&
              runtime.bytes[0x5000u] == 0u &&
              runtime.frees == (std::vector<std::uint32_t>{0u, 0u}),
          "flag zero cleans without deleting entry or counted buffer");
    runtime = Runtime{};
    runtime.words[entry + 8u] = 0x5001u;
    runtime.bytes[0x5000u] = 0u;
    Check(Entry::Destroy(entry, 1u, runtime) == entry &&
              runtime.frees ==
                  (std::vector<std::uint32_t>{0u, 0u, 0x5000u, entry}),
          "zero count frees buffer and deleting destructor frees entry last");
    pal4::ArenaCacheEntryRuntimeStub stub;
    Check(Entry::Initialize(entry, stub) == entry,
          "native memory boundary stub is linkable");
}

void ClearChildren() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    runtime.words[entry + 40u] = 0x1000u;
    runtime.words[entry + 44u] = 0x100cu;
    runtime.words[0x1000u] = 0x4000u;
    runtime.words[0x1004u] = 0u;
    runtime.words[0x1008u] = 0x5000u;
    runtime.words[entry + 56u] = 0x2000u;
    runtime.words[entry + 60u] = 0x2008u;
    runtime.words[0x2000u] = 0x6000u;
    runtime.words[0x2004u] = 0u;
    runtime.words[entry + 68u] = 0x7000u;
    Check(Entry::ClearChildren(entry, runtime) == 0x7100u &&
              runtime.words[entry + 44u] == 0x1000u &&
              runtime.words[entry + 60u] == 0x2000u &&
              runtime.words[entry + 68u] == 0u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Op::DeleteChild, {0x4000u, 1u}},
                  {Op::DeleteChild, {0x5000u, 1u}},
                  {Op::DeleteChild, {0x6000u, 1u}},
                  {Op::DeleteChild, {0x7000u, 1u}}}),
          "children are deleted in array order before both ends and extra reset");
    runtime = Runtime{};
    runtime.words[entry + 40u] = 0x1000u;
    runtime.words[entry + 44u] = 0x1000u;
    runtime.words[entry + 56u] = 0x2000u;
    runtime.words[entry + 60u] = 0x2000u;
    Check(Entry::ClearChildren(entry, runtime) == 0x2000u &&
              runtime.calls.empty(),
          "empty arrays with no extra return original second end");
}

void MatchNames() {
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    runtime.words[entry + 8u] = 0x7000u;
    runtime.words[entry + 24u] = 0x7100u;
    runtime.names[0x7000u] = "World";
    runtime.names[0x7100u] = "Area";
    runtime.names[0x7200u] = "wOrLd";
    runtime.names[0x7300u] = "aReA";
    runtime.names[0x7400u] = "Other";
    Check(Entry::MatchesNames(entry, 0x7200u, 0x7300u, runtime) == 1u &&
              runtime.comparisons == 2u,
          "both names compare without case");
    runtime.comparisons = 0u;
    Check(Entry::MatchesNames(entry, 0x7400u, 0x7300u, runtime) == 0u &&
              runtime.comparisons == 1u,
          "first mismatch short-circuits second comparison");
    runtime.words[entry + 8u] = 0u;
    runtime.words[entry + 24u] = 0u;
    runtime.names[0x7500u] = "";
    Check(Entry::MatchesNames(entry, 0x7500u, 0x7500u, runtime) == 1u,
          "null source names use empty-string fallback");
}

void ProcessChildren() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    constexpr std::uint32_t output = 0x9000u;
    runtime.words[entry + 40u] = 0x1000u;
    runtime.words[entry + 44u] = 0x100cu;
    runtime.words[0x1000u] = 0x4000u;
    runtime.words[0x1004u] = 0u;
    runtime.words[0x1008u] = 0x5000u;
    runtime.words[entry + 56u] = 0x2000u;
    runtime.words[entry + 60u] = 0x2004u;
    runtime.words[0x2000u] = 0x6000u;
    runtime.words[entry + 68u] = 0x7000u;
    Check(Entry::ProcessChildren(entry, output, runtime) == 0x7002u &&
              runtime.calls == (std::vector<Runtime::Invocation>{
                  {Op::PrepareData, {output, entry + 4u}},
                  {Op::PrepareData, {output, entry + 20u}},
                  {Op::WriteData, {output, 2u, 4u}},
                  {Op::GetChildValue, {0x4000u}},
                  {Op::WriteData, {output, 0x4001u, 4u}},
                  {Op::ProcessChild, {0x4000u, output}},
                  {Op::GetChildValue, {0x5000u}},
                  {Op::WriteData, {output, 0x5001u, 4u}},
                  {Op::ProcessChild, {0x5000u, output}},
                  {Op::WriteData, {output, 1u, 4u}},
                  {Op::GetChildValue, {0x6000u}},
                  {Op::WriteData, {output, 0x6001u, 4u}},
                  {Op::ProcessChild, {0x6000u, output}},
                  {Op::ProcessChild, {0x7000u, output}}}),
          "both arrays emit counts, values and callbacks in IDA order");
    runtime = Runtime{};
    runtime.words[entry + 40u] = 0x1000u;
    runtime.words[entry + 44u] = 0x1000u;
    runtime.words[entry + 56u] = 0x2000u;
    runtime.words[entry + 60u] = 0x2000u;
    Check(Entry::ProcessChildren(entry, output, runtime) == 0x2000u &&
              runtime.calls.size() == 4u,
          "empty lists emit zero counts and return second end");
}

void LoadDataAndSkipNegativeIds() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    constexpr std::uint32_t data = 0x7000u;
    runtime.input_words = {3u, 7u, 0xffffffffu, 9u,
                           2u, 0xffffffffu, 4u};
    Check(Entry::Load(entry, data, runtime) == 0x300cu,
          "load returns minimap deserialization result");
    Check(runtime.input_words.empty() &&
              runtime.allocation_sizes ==
                  (std::vector<std::uint32_t>{4u, 8u, 4u}) &&
              runtime.words[entry + 40u] == 0xa100u &&
              runtime.words[entry + 44u] == 0xa108u &&
              runtime.words[0xa100u] == 0x1007u &&
              runtime.words[0xa104u] == 0x1009u &&
              runtime.words[entry + 56u] == 0xa200u &&
              runtime.words[0xa200u] == 0x1004u &&
              runtime.words[entry + 68u] == 0x100cu,
          "load stores two filtered arrays and minimap in IDA layout");
    Check(runtime.calls == (std::vector<Runtime::Invocation>{
              {Op::ReadTextRecord, {data, entry + 4u}},
              {Op::ReadTextRecord, {data, entry + 20u}},
              {Op::ReadDataWord, {data, 4u}},
              {Op::ReadDataWord, {data, 4u}},
              {Op::FindStorageObject, {7u}},
              {Op::DeserializeObject, {0x1007u, data}},
              {Op::ReadDataWord, {data, 4u}},
              {Op::ReadDataWord, {data, 4u}},
              {Op::FindStorageObject, {9u}},
              {Op::DeserializeObject, {0x1009u, data}},
              {Op::ReadDataWord, {data, 4u}},
              {Op::ReadDataWord, {data, 4u}},
              {Op::ReadDataWord, {data, 4u}},
              {Op::FindStorageObject, {4u}},
              {Op::DeserializeObject, {0x1004u, data}},
              {Op::FindStorageObject, {12u}},
              {Op::DeserializeObject, {0x100cu, data}}}),
          "negative IDs skip lookup and deserialize while reads remain ordered");

    runtime = Runtime{};
    runtime.input_words = {0u, 0xffffffffu};
    Check(Entry::Load(entry, data, runtime) == 0x300cu &&
              runtime.allocation_sizes.empty() &&
              runtime.calls.size() == 6u,
          "nonpositive counts leave both child arrays empty");
}

void CaptureNamesAndCallbacks() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    runtime.bytes[0x1000u] = 'A';
    runtime.bytes[0x1001u] = 0u;
    runtime.bytes[0x2000u] = 'B';
    runtime.bytes[0x2001u] = 'C';
    runtime.bytes[0x2002u] = 0u;
    runtime.iterate_objects = [&](Runtime& current) {
        Check(Entry::CaptureGameObject(0x3000u, entry, current) == 0x3000u,
              "object callback returns its source object");
        Check(Entry::CaptureGameObject(0x3004u, entry, current) == 0x3004u,
              "second object callback returns its source object");
    };
    runtime.iterate_monsters = [&](Runtime& current) {
        Check(Entry::CaptureMonster(0x4000u, entry, current) == 0x4000u,
              "monster callback returns its source object");
    };
    Check(Entry::Capture(entry, 0x1000u, 0x2000u, runtime) == 0x9000u &&
              runtime.words[entry + 12u] == 1u &&
              runtime.words[entry + 28u] == 2u &&
              runtime.bytes[runtime.words[entry + 8u]] == 'A' &&
              runtime.bytes[runtime.words[entry + 24u] + 1u] == 'C' &&
              runtime.words[entry + 68u] == 0x9000u &&
              runtime.words[runtime.words[entry + 40u]] == 0x3100u &&
              runtime.words[runtime.words[entry + 40u] + 4u] == 0x3104u &&
              runtime.words[runtime.words[entry + 56u]] == 0x4200u,
          "capture copies both strings and collects callback snapshots");
    Check(runtime.allocation_sizes ==
              (std::vector<std::uint32_t>{4u, 8u, 4u}) &&
              runtime.calls[0] == Runtime::Invocation{Op::AllocateText,
                                                      {entry + 4u, 1u, 1u}} &&
              runtime.calls[1] == Runtime::Invocation{Op::AllocateText,
                                                      {entry + 20u, 2u, 1u}} &&
              runtime.calls[2] == Runtime::Invocation{Op::IterateGameObjects,
                                                      {entry}} &&
              runtime.calls[5] == Runtime::Invocation{Op::IterateMonsters,
                                                      {entry}} &&
              runtime.calls.back() == Runtime::Invocation{Op::CaptureMinimap,
                                                          {}},
          "capture follows text, object, monster, minimap order");
}

void RestoreByName() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t entry = 0x6000u;
    runtime.words[entry + 40u] = 0x1000u;
    runtime.words[entry + 44u] = 0x100cu;
    runtime.words[0x1000u] = 0x2000u;
    runtime.words[0x1004u] = 0u;
    runtime.words[0x1008u] = 0x2100u;
    runtime.words[entry + 56u] = 0x1100u;
    runtime.words[entry + 60u] = 0x1104u;
    runtime.words[0x1100u] = 0x3000u;
    runtime.words[entry + 68u] = 0x4000u;
    runtime.child_names = {{0x2000u, 0x5000u},
                           {0x2100u, 0x5100u},
                           {0x3000u, 0x5200u}};
    runtime.live_objects = {{0x5000u, 0x6000u}, {0x5200u, 0x7000u}};
    Entry::Restore(entry, runtime);
    Check(runtime.calls == (std::vector<Runtime::Invocation>{
              {Op::GetChildName, {0x2000u}},
              {Op::FindGameObjectByName, {0x5000u}},
              {Op::RestoreGameObject, {0x6000u, 0x2000u}},
              {Op::GetChildName, {0x2100u}},
              {Op::FindGameObjectByName, {0x5100u}},
              {Op::GetChildName, {0x3000u}},
              {Op::FindMonsterByName, {0x5200u}},
              {Op::RestoreMonster, {0x7000u, 0x3000u}},
              {Op::RestoreMinimap, {0x4000u}}}),
          "restore skips null and missing children, then restores minimap");
    runtime = Runtime{};
    Entry::Restore(entry, runtime);
    Check(runtime.calls.empty(), "empty cache does not call managers");
}

void CopyConstructEntry() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t source = 0x6000u;
    constexpr std::uint32_t target = 0x7000u;
    runtime.bytes[source + 4u] = 0x11u;
    runtime.bytes[source + 20u] = 0x22u;
    runtime.bytes[source + 36u] = 0x33u;
    runtime.bytes[source + 52u] = 0x44u;
    runtime.words[source + 40u] = 0x1000u;
    runtime.words[source + 44u] = 0x1008u;
    runtime.words[0x1000u] = 0xaaaa0001u;
    runtime.words[0x1004u] = 0u;
    runtime.words[source + 56u] = 0x2000u;
    runtime.words[source + 60u] = 0x2004u;
    runtime.words[0x2000u] = 0xbbbb0002u;
    runtime.words[source + 68u] = 0xcccc0003u;
    Entry::CopyConstruct(target, source, runtime);
    Check(runtime.calls == (std::vector<Runtime::Invocation>{
              {Op::AssignTextRecord, {target + 4u, source + 4u}},
              {Op::AssignTextRecord, {target + 20u, source + 20u}}}) &&
              runtime.allocation_sizes ==
                  (std::vector<std::uint32_t>{8u, 4u}) &&
              runtime.bytes[target + 4u] == 0x11u &&
              runtime.bytes[target + 20u] == 0x22u &&
              runtime.bytes[target + 36u] == 0x33u &&
              runtime.bytes[target + 52u] == 0x44u &&
              runtime.words[target + 40u] == 0xa000u &&
              runtime.words[target + 44u] == 0xa008u &&
              runtime.words[target + 48u] == 0xa008u &&
              runtime.words[0xa000u] == 0xaaaa0001u &&
              runtime.words[0xa004u] == 0u &&
              runtime.words[target + 56u] == 0xa100u &&
              runtime.words[target + 60u] == 0xa104u &&
              runtime.words[target + 64u] == 0xa104u &&
              runtime.words[0xa100u] == 0xbbbb0002u &&
              runtime.words[target + 68u] == 0xcccc0003u &&
              runtime.words[target] == Entry::kVtable,
          "copy constructor duplicates both pointer arrays and final child");
    runtime.calls.clear();
    runtime.allocation_sizes.clear();
    Entry::CopyConstruct(0u, source, runtime);
    Check(runtime.calls.empty() && runtime.allocation_sizes.empty(),
          "null destination skips copy construction");
}

}  // namespace

int main() {
    Construction();
    CleanupCountedAndOwned();
    DeletionFlagsAndCounts();
    ClearChildren();
    MatchNames();
    ProcessChildren();
    LoadDataAndSkipNegativeIds();
    CaptureNamesAndCallbacks();
    RestoreByName();
    CopyConstructEntry();
}
