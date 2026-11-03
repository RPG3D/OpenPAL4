#include "pal4/game_data_processor.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IGameDataProcessorRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations;
    std::vector<std::uint32_t> frees;
    std::uint32_t primary = 0x5000u;
    std::uint32_t secondary = 0x6014u;
    std::uint32_t next_allocation = 0x9000u;

    std::uint32_t GetGameDatabase() override {
        calls.push_back("database");
        return 0x2000u;
    }
    std::uint32_t LookupPrimary(std::uint32_t collection,
                                std::uint32_t id) override {
        calls.push_back("primary:" + std::to_string(collection) + ":" +
                        std::to_string(id));
        return primary;
    }
    std::uint32_t LookupSoundPrimary(std::uint32_t collection,
                                     std::uint32_t id) override {
        calls.push_back("sound:" + std::to_string(collection) + ":" +
                        std::to_string(id));
        return primary;
    }
    std::uint32_t LookupSecondary(std::uint32_t collection,
                                  std::uint32_t id) override {
        calls.push_back("secondary:" + std::to_string(collection) + ":" +
                        std::to_string(id));
        return secondary;
    }
    std::uint32_t ReadDword(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words.at(address);
    }
    void WriteDword(std::uint32_t address, std::uint32_t value) override {
        calls.push_back("write:" + std::to_string(address) + ":" +
                        std::to_string(value));
        words[address] = value;
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        allocations.push_back(bytes);
        return next_allocation;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
        frees.push_back(address);
    }
};

using Processor = pal4::GameDataProcessor;
using Result = pal4::IndexedFilteredResultArrayState;

Runtime MakeRuntime() {
    Runtime runtime;
    runtime.words = {{0x5048u, 77u}, {0x6014u, 0xaabbccddu},
                     {0x6020u, 0xffffffffu}};
    return runtime;
}

void TestLookupGates() {
    Result result{0x55u, 0x7000u, 0x7000u, 0x7004u};
    Runtime runtime = MakeRuntime();
    runtime.primary = 0u;
    Check(Processor::ProcessAndInsert(result, 12u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "database", "primary:8204:12"} &&
              result.end == 0x7000u,
          "missing primary record stops after first database lookup");

    runtime = MakeRuntime();
    runtime.secondary = 0u;
    Check(Processor::ProcessAndInsert(result, 12u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "database", "primary:8204:12", "database",
                  "read:20552", "secondary:8204:77"} &&
              result.end == 0x7000u,
          "missing secondary record stops before sentinel read");

    runtime = MakeRuntime();
    runtime.words[0x6020u] = 0u;
    Check(Processor::ProcessAndInsert(result, 12u, runtime) == 0x6000u &&
              runtime.calls == std::vector<std::string>{
                  "database", "primary:8204:12", "database",
                  "read:20552", "secondary:8204:77", "read:24608"} &&
              result.end == 0x7000u,
          "non-sentinel secondary record preserves EAX high bytes and skips append");
}

void TestSpareCapacity() {
    Result result{0x55u, 0x7000u, 0x7004u, 0x700cu};
    Runtime runtime = MakeRuntime();
    runtime.words[0x7000u] = 3u;
    Check(Processor::ProcessAndInsert(result, 12u, runtime) == 0x7004u &&
              result.reserved == 0x55u && result.begin == 0x7000u &&
              result.end == 0x7008u && result.capacity_end == 0x700cu &&
              runtime.words[0x7000u] == 3u &&
              runtime.words[0x7004u] == 0xaabbccddu &&
              runtime.allocations.empty() && runtime.frees.empty() &&
              runtime.calls == std::vector<std::string>{
                  "database", "primary:8204:12", "database",
                  "read:20552", "secondary:8204:77", "read:24608",
                  "read:24596", "write:28676:2864434397"},
          "accepted record appends its first DWORD in spare capacity");
}

void TestGrowth() {
    Result result;
    Runtime runtime = MakeRuntime();
    Check(Processor::ProcessAndInsert(result, 12u, runtime) == 0x9004u &&
              result.begin == 0x9000u && result.end == 0x9004u &&
              result.capacity_end == 0x9004u &&
              runtime.allocations == std::vector<std::uint32_t>{4u} &&
              runtime.frees == std::vector<std::uint32_t>{0u} &&
              runtime.words[0x9000u] == 0xaabbccddu,
          "empty target allocates one DWORD and returns new end");

    runtime = MakeRuntime();
    runtime.next_allocation = 0xa000u;
    result = {0x77u, 0x7000u, 0x7008u, 0x7008u};
    runtime.words[0x7000u] = 11u;
    runtime.words[0x7004u] = 22u;
    Check(Processor::ProcessAndInsert(result, 12u, runtime) == 0xa00cu &&
              result.reserved == 0x77u && result.begin == 0xa000u &&
              result.end == 0xa00cu && result.capacity_end == 0xa010u &&
              runtime.allocations == std::vector<std::uint32_t>{16u} &&
              runtime.frees == std::vector<std::uint32_t>{0x7000u} &&
              runtime.words[0xa000u] == 11u &&
              runtime.words[0xa004u] == 22u &&
              runtime.words[0xa008u] == 0xaabbccddu,
          "full two-entry target grows to four and preserves old values");
}

void TestSoundPrimaryVariant() {
    Result result{0u, 0x7000u, 0x7000u, 0x7004u};
    Runtime runtime = MakeRuntime();
    runtime.words[0x5070u] = 88u;
    Check(Processor::ProcessSoundAndInsert(result, 13u, runtime) == 0x7000u &&
              result.end == 0x7004u &&
              runtime.words[0x7000u] == 0xaabbccddu &&
              runtime.calls == std::vector<std::string>{
                  "database", "sound:8204:13", "database",
                  "read:20592", "secondary:8204:88", "read:24608",
                  "read:24596", "write:28672:2864434397"},
          "sound variant uses its own lookup and primary +0x70 key");
    runtime = MakeRuntime();
    runtime.primary = 0u;
    Check(Processor::ProcessSoundAndInsert(result, 13u, runtime) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "database", "sound:8204:13"} &&
              result.end == 0x7004u,
          "sound variant stops after a missing first-layer record");
}

}  // namespace

int main() {
    TestLookupGates();
    TestSpareCapacity();
    TestGrowth();
    TestSoundPrimaryVariant();
    Result result;
    pal4::GameDataProcessorRuntimeStub stub;
    Check(Processor::ProcessAndInsert(result, 0u, stub) == 0u,
          "default GameData processor runtime links");
    std::cout << "5 game data processor cases passed\n";
}
