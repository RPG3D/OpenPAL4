#include "pal4/game_data_combat_prize_table_capacity.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kTable = 0x2000;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataCombatPrizeTableCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t next = 0xa000;

    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate:" + std::to_string(bytes));
        const auto result = next;
        next += 0x1000;
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free:" + std::to_string(address));
    }
    void CopyRecord(std::uint32_t destination,
                    std::uint32_t source) override {
        for (std::uint32_t offset = 0; offset < 128; offset += 4)
            words[destination + offset] = words[source + offset];
        calls.push_back("record:" + std::to_string(source));
    }
    void CopyIndex(std::uint32_t destination,
                   std::uint32_t source) override {
        words[destination] = words[source];
        words[destination + 4] = words[source + 4];
        calls.push_back("index:" + std::to_string(source));
    }
};

Runtime Base() {
    Runtime runtime;
    runtime.words[kTable + 96] = 0x4000;
    runtime.words[kTable + 100] = 0x4100;
    runtime.words[kTable + 104] = 0x4180;
    runtime.words[kTable + 8] = 0x6000;
    runtime.words[kTable + 12] = 0x6010;
    runtime.words[kTable + 16] = 0x6018;
    runtime.words[0x4000] = 11;
    runtime.words[0x407c] = 12;
    runtime.words[0x4080] = 21;
    runtime.words[0x40fc] = 22;
    runtime.words[0x6000] = 31;
    runtime.words[0x6004] = 32;
    runtime.words[0x6008] = 41;
    runtime.words[0x600c] = 42;
    return runtime;
}

void CheckGrowthModes() {
    auto unchanged = Base();
    Check(pal4::GameDataCombatPrizeTableCapacity::Reserve(
              kTable, 3, unchanged) == 3 && unchanged.calls.empty(),
          "unchanged index returns capacity without mutations");

    auto record_only = Base();
    record_only.words[kTable + 16] = 0x6030;
    Check(pal4::GameDataCombatPrizeTableCapacity::Reserve(
              kTable, 4, record_only) == 6 &&
              record_only.words[kTable + 96] == 0xa000 &&
              record_only.words[kTable + 100] == 0xa100 &&
              record_only.words[kTable + 104] == 0xa200 &&
              record_only.words[0xa000] == 11 &&
              record_only.words[0xa07c] == 12 &&
              record_only.words[0xa080] == 21 &&
              record_only.words[0xa0fc] == 22 &&
              record_only.calls[0] == "allocate:512" &&
              record_only.calls[1] == "record:16384" &&
              record_only.calls[2] == "record:16512" &&
              record_only.calls[3] == "free:16384",
          "record-only growth copies 128-byte records and retains index");

    auto index_only = Base();
    index_only.words[kTable + 104] = 0x4280;
    Check(pal4::GameDataCombatPrizeTableCapacity::Reserve(
              kTable, 4, index_only) == 2 &&
              index_only.words[kTable + 8] == 0xa000 &&
              index_only.words[kTable + 12] == 0xa010 &&
              index_only.words[kTable + 16] == 0xa020 &&
              index_only.words[0xa000] == 31 &&
              index_only.words[0xa00c] == 42 &&
              index_only.calls[0] == "allocate:32" &&
              index_only.calls[1] == "index:24576" &&
              index_only.calls[2] == "index:24584" &&
              index_only.calls[3] == "free:24576",
          "index-only growth returns old live entry count");

    auto combined = Base();
    Check(pal4::GameDataCombatPrizeTableCapacity::Reserve(
              kTable, 5, combined) == 2 &&
              combined.words[kTable + 96] == 0xa000 &&
              combined.words[kTable + 100] == 0xa100 &&
              combined.words[kTable + 104] == 0xa280 &&
              combined.words[kTable + 8] == 0xb000 &&
              combined.words[kTable + 12] == 0xb010 &&
              combined.words[kTable + 16] == 0xb028 &&
              combined.words[0xb000] == 31,
          "combined growth retains both live array lengths");
}

void CheckEmptyGrowth() {
    Runtime runtime;
    Check(pal4::GameDataCombatPrizeTableCapacity::Reserve(
              kTable, 2, runtime) == 0 &&
              runtime.words[kTable + 96] == 0xa000 &&
              runtime.words[kTable + 100] == 0xa000 &&
              runtime.words[kTable + 104] == 0xa100 &&
              runtime.words[kTable + 8] == 0xb000 &&
              runtime.words[kTable + 12] == 0xb000 &&
              runtime.words[kTable + 16] == 0xb010,
          "empty arrays allocate independently and return zero copied keys");
}

}  // namespace

int main() {
    try {
        CheckGrowthModes();
        CheckEmptyGrowth();
        std::cout << "game_data_combat_prize_table_capacity_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
