#include "pal4/arena_cache.h"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

using Cache = pal4::ArenaCache;

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IArenaCacheEntryRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, std::uint8_t> bytes;
    std::vector<std::uint32_t> frees;
    std::vector<std::uint32_t> allocations;
    std::deque<std::uint32_t> input_words;
    std::vector<std::uint32_t> deleted_children;
    std::vector<pal4::ArenaCacheEntryCall> calls;
    struct Invocation {
        pal4::ArenaCacheEntryCall operation;
        std::vector<std::uint32_t> arguments;
        bool operator==(const Invocation&) const = default;
    };
    std::vector<Invocation> invocations;
    std::map<std::uint32_t, std::string> names;
    std::uint32_t next_allocation = 0x7e006000u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
    }
    std::uint8_t ReadByte(std::uint32_t address) override {
        return bytes[address];
    }
    void WriteByte(std::uint32_t address, std::uint8_t value) override {
        bytes[address] = value;
    }
    std::uint32_t Free(std::uint32_t address) override {
        frees.push_back(address);
        return address + 0x10000u;
    }
    std::uint32_t Allocate(std::uint32_t bytes_count) override {
        allocations.push_back(bytes_count);
        const auto result = next_allocation;
        next_allocation += 0x1000u;
        return result;
    }
    bool NamesEqualIgnoreCase(std::uint32_t left,
                              std::uint32_t right) override {
        return names[left] == names[right];
    }
    std::uint32_t Call(pal4::ArenaCacheEntryCall operation,
                       std::initializer_list<std::uint32_t> arguments) override {
        calls.push_back(operation);
        invocations.push_back({operation, arguments});
        if (operation == pal4::ArenaCacheEntryCall::RegisterArenaCacheSingleton)
            return 0x5000u;
        if (operation == pal4::ArenaCacheEntryCall::ReadDataWord) {
            Check(!input_words.empty(), "cache data stream has next word");
            const auto value = input_words.front();
            input_words.pop_front();
            return value;
        }
        if (operation == pal4::ArenaCacheEntryCall::FindStorageObject)
            return *arguments.begin() + 0x1000u;
        if (operation == pal4::ArenaCacheEntryCall::DeleteChild) {
            deleted_children.push_back(*arguments.begin());
            return *arguments.begin() + 1u;
        }
        return 0u;
    }
};

void SingletonRegistrationAndFactory() {
    Runtime runtime;
    std::uint8_t guard = 0x80u;
    std::uint32_t registered = 0u;
    Check(Cache::Register(guard, registered, runtime) == 0x5000u &&
              guard == 0x81u && registered == 0x5000u &&
              runtime.calls.size() == 1u,
          "singleton registration sets guard and stores instance");
    Check(Cache::Register(guard, registered, runtime) == 0x5000u &&
              runtime.calls.size() == 1u,
          "repeated registration returns existing singleton");
    const auto cache = Cache::Create(runtime);
    Check(cache == 0x7e006000u && runtime.allocations ==
              (std::vector<std::uint32_t>{20u}) &&
              runtime.words[cache] == 1u &&
              runtime.bytes[cache + 4u] == 0x7eu &&
              runtime.words[cache + 8u] == 0u &&
              runtime.words[cache + 12u] == 0u &&
              runtime.words[cache + 16u] == 0u,
          "factory allocates 20-byte manager with saved-pointer high byte");
    runtime.next_allocation = 0u;
    Check(Cache::Create(runtime) == 0u,
          "factory returns null when allocation fails");
}

void ReleaseAndDestroyEntries() {
    Runtime runtime;
    constexpr std::uint32_t cache = 0x6000u;
    constexpr std::uint32_t begin = 0x1000u;
    runtime.words[cache + 8u] = begin;
    runtime.words[cache + 12u] = begin + 2u * Cache::kEntrySize;
    runtime.words[cache + 16u] = begin + 3u * Cache::kEntrySize;
    runtime.words[begin + 8u] = 0x3001u;
    runtime.bytes[0x3000u] = 0xffu;
    Check(Cache::ReleaseEntries(cache, runtime) == 0x11000u &&
              runtime.words[cache] == 1u &&
              runtime.words[cache + 8u] == 0u &&
              runtime.words[cache + 12u] == 0u &&
              runtime.words[cache + 16u] == 0u,
          "release destroys 72-byte entries then clears array triple");
    Check(runtime.frees.back() == begin &&
              runtime.frees[runtime.frees.size() - 2u] == 0u &&
              runtime.frees[runtime.frees.size() - 3u] == 0u,
          "backing array is freed after entries");
    runtime = Runtime{};
    Check(Cache::Destroy(0u, runtime) == 0u && runtime.frees.empty(),
          "null deletion does not access memory");
    Check(Cache::Destroy(cache, runtime) == cache + 0x10000u &&
              runtime.frees == (std::vector<std::uint32_t>{0u, cache}),
          "deletion callback releases array before manager");
}

void ClearLiveEntries() {
    Runtime runtime;
    constexpr std::uint32_t cache = 0x6000u;
    constexpr std::uint32_t begin = 0x1000u;
    runtime.words[cache + 8u] = begin;
    runtime.words[cache + 12u] = begin + 2u * Cache::kEntrySize;
    runtime.words[cache + 16u] = begin + 3u * Cache::kEntrySize;
    runtime.words[begin + 68u] = 0x5000u;
    runtime.words[begin + Cache::kEntrySize + 68u] = 0x5100u;
    Check(Cache::ClearEntries(cache, runtime) ==
              begin + Cache::kEntrySize &&
              runtime.deleted_children ==
                  (std::vector<std::uint32_t>{0x5000u, 0x5100u}) &&
              runtime.words[cache + 8u] == begin &&
              runtime.words[cache + 12u] == begin &&
              runtime.words[cache + 16u] == begin + 3u * Cache::kEntrySize &&
              runtime.words[begin + 68u] == 0u,
          "clear releases children then entries while retaining allocation");
    runtime = Runtime{};
    runtime.words[cache + 8u] = begin;
    runtime.words[cache + 12u] = begin;
    Check(Cache::ClearEntries(cache, runtime) == begin &&
              runtime.calls.empty(),
          "empty cache performs no child or destructor calls");
}

void CaptureAndRestoreNamedEntries() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t cache = 0x6000u;
    constexpr std::uint32_t entry = 0x1000u;
    runtime.words[cache] = 1u;
    runtime.words[cache + 8u] = entry;
    runtime.words[cache + 12u] = entry + Cache::kEntrySize;
    runtime.words[entry + 8u] = 0x5000u;
    runtime.words[entry + 24u] = 0x5100u;
    runtime.names = {{0x5000u, "arena"}, {0x5100u, "room"},
                     {0x5200u, "arena"}, {0x5300u, "room"}};
    Check(Cache::CaptureOrInsert(cache, 0x5200u, 0x5300u, 0x7000u,
                                 runtime) == 1u &&
              runtime.words[cache + 12u] == entry + Cache::kEntrySize &&
              runtime.calls == (std::vector<Op>{
                  Op::AllocateText, Op::AllocateText,
                  Op::IterateGameObjects, Op::IterateMonsters,
                  Op::CaptureMinimap}),
          "matching entry is recaptured without insertion or log");
    runtime.calls.clear();
    runtime.invocations.clear();
    Check(Cache::Restore(cache, 0x5200u, 0x5300u, runtime) == 1u &&
              runtime.invocations == (std::vector<Runtime::Invocation>{
                  {Op::LogCacheEvent, {102u, 0x5200u, 0x5300u}}}),
          "matching entry restores and emits success log");
    runtime.names[0x5300u] = "other";
    runtime.invocations.clear();
    Check(Cache::Restore(cache, 0x5200u, 0x5300u, runtime) == 0u &&
              runtime.invocations == (std::vector<Runtime::Invocation>{
                  {Op::LogCacheEvent, {95u, 0x5200u, 0x5300u}}}),
          "missing entry emits miss log");
    runtime.words[cache] = 0u;
    runtime.invocations.clear();
    Check(Cache::Restore(cache, 0x5200u, 0x5300u, runtime) == 0u &&
              Cache::CaptureOrInsert(cache, 0x5200u, 0x5300u, 0x7000u,
                                     runtime) == 0u &&
              runtime.invocations.empty(),
          "disabled cache performs no capture, lookup or log");
}

void InsertAndSerialize() {
    using Op = pal4::ArenaCacheEntryCall;
    Runtime runtime;
    constexpr std::uint32_t cache = 0x6000u;
    constexpr std::uint32_t temporary = 0x7000u;
    runtime.words[cache] = 1u;
    Check(Cache::CaptureOrInsert(cache, 0x5000u, 0x5100u, temporary,
                                 runtime) == 1u &&
              runtime.words[temporary] == pal4::ArenaCacheEntry::kVtable &&
              runtime.words[cache + 8u] == 0x7e006000u &&
              runtime.words[cache + 12u] ==
                  0x7e006000u + Cache::kEntrySize &&
              runtime.words[0x7e006000u] ==
                  pal4::ArenaCacheEntry::kVtable &&
              runtime.allocations ==
                  (std::vector<std::uint32_t>{72u, 0u, 0u}) &&
              runtime.invocations[7] == Runtime::Invocation{
                  Op::LogCacheEvent, {63u, 0x5000u, 0x5100u}},
          "new entry is copied into manager array before log and scratch cleanup");

    runtime = Runtime{};
    constexpr std::uint32_t entry = 0x1000u;
    runtime.words[cache] = 1u;
    runtime.words[cache + 8u] = entry;
    runtime.words[cache + 12u] = entry + Cache::kEntrySize;
    runtime.words[entry + 8u] = 0x5200u;
    runtime.words[entry + 24u] = 0x5300u;
    runtime.names = {{0x5200u, "arena"}, {0x5300u, "room"},
                     {0x5000u, "arena"}, {0x5100u, "room"}};
    Check(Cache::Save(cache, 0x5000u, 0x5100u, 0x9000u,
                      temporary, runtime) == 0u,
          "save forwards the last entry serialization result");
    Check(runtime.invocations[5] == Runtime::Invocation{
              Op::WriteDataWord, {0x9000u, 1u, 4u}} &&
              runtime.invocations[6] == Runtime::Invocation{
                  Op::PrepareData, {0x9000u, entry + 4u}} &&
              runtime.invocations[7] == Runtime::Invocation{
                  Op::PrepareData, {0x9000u, entry + 20u}},
          "save captures first, writes 72-byte entry count, then serializes");
    runtime.words[cache] = 0u;
    runtime.invocations.clear();
    Check(Cache::Save(cache, 0x5000u, 0x5100u, 0x9000u,
                      temporary, runtime) == 0u &&
              runtime.invocations.empty(),
          "disabled cache does not write output");
}

void EntryInsertionAndLoad() {
    Runtime runtime;
    constexpr std::uint32_t cache = 0x6000u;
    constexpr std::uint32_t source = 0x7000u;
    runtime.words[source + 68u] = 0x4444u;
    Check(Cache::InsertEntries(cache, 0u, 2u, source, runtime) ==
              0x7e006000u + 2u * Cache::kEntrySize &&
              runtime.allocations.front() == 2u * Cache::kEntrySize &&
              runtime.words[cache + 8u] == 0x7e006000u &&
              runtime.words[cache + 12u] == runtime.words[cache + 16u] &&
              runtime.words[0x7e006000u + 68u] == 0x4444u &&
              runtime.words[0x7e006000u + Cache::kEntrySize + 68u] ==
                  0x4444u,
          "multi-entry insertion copies each 72-byte record");
    const auto begin = runtime.words[cache + 8u];
    runtime.words[cache + 16u] += Cache::kEntrySize;
    Check(Cache::InsertEntries(cache, begin + Cache::kEntrySize, 1u,
                               source, runtime) ==
              begin + 3u * Cache::kEntrySize &&
              runtime.words[cache + 8u] == begin &&
              runtime.words[begin + Cache::kEntrySize + 68u] == 0x4444u &&
              runtime.words[begin + 2u * Cache::kEntrySize + 68u] ==
                  0x4444u,
          "spare-capacity insertion shifts trailing entries in place");

    runtime = Runtime{};
    runtime.words[cache] = 1u;
    runtime.input_words = {2u, 0u, 0u, 0u, 0u};
    const auto load_result = Cache::Load(cache, 0x9000u, source, runtime);
    const auto loaded_begin = runtime.words[cache + 8u];
    Check(load_result == runtime.words[cache + 12u],
          "load returns array end");
    Check(runtime.input_words.empty(), "load consumes entry and child counts");
    Check(runtime.words[cache + 12u] - loaded_begin == 2u * Cache::kEntrySize,
          "load inserts two entries");
    Check(runtime.words[loaded_begin + 68u] == 0x100cu,
          "load resolves first minimap");
    Check(runtime.words[loaded_begin + Cache::kEntrySize + 68u] == 0x100cu,
          "load resolves second minimap");
    runtime.words[cache] = 0u;
    runtime.input_words.clear();
    Check(Cache::Load(cache, 0x9000u, source, runtime) == 0u,
          "disabled cache does not read serialized input");
}

}  // namespace

int main() {
    SingletonRegistrationAndFactory();
    ReleaseAndDestroyEntries();
    ClearLiveEntries();
    CaptureAndRestoreNamedEntries();
    InsertAndSerialize();
    EntryInsertionAndLoad();
}
