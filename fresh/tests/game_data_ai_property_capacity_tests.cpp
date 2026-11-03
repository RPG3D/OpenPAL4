#include "pal4/game_data_ai_record_capacity.h"

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

struct Runtime final : pal4::IGameDataAiPropertyCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GameDataAiPropertyRecord> records;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x6000, 0x7000};

    Runtime() {
        words[0x1060] = 0x4000;
        words[0x1064] = 0x4000 + 2 * 68;
        words[0x1068] = 0x4000 + 3 * 68;
        words[0x1008] = 0x5000;
        words[0x100c] = 0x5010;
        words[0x1010] = 0x5000 + 3 * 8;
        for (std::uint32_t i = 0; i < 2; ++i) {
            pal4::GameDataAiPropertyRecord record{};
            record[0] = 10 + i;
            record[16] = 100 + i;
            records[0x4000 + i * 68] = record;
            keys[0x5000 + i * 8] = {10 + i, i};
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        Check(!allocations.empty(), "unexpected allocation");
        auto result = allocations.front();
        allocations.erase(allocations.begin());
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    pal4::GameDataAiPropertyRecord ReadRecord(
        std::uint32_t address) override {
        return records.at(address);
    }
    void WriteRecord(
        std::uint32_t address,
        const pal4::GameDataAiPropertyRecord& record) override {
        calls.push_back("copy record " + std::to_string(address));
        records[address] = record;
    }
    pal4::GameDataAiKeyEntry ReadKey(std::uint32_t address) override {
        return keys.at(address);
    }
    void WriteKey(std::uint32_t address,
                  const pal4::GameDataAiKeyEntry& entry) override {
        calls.push_back("copy key " + std::to_string(address));
        keys[address] = entry;
    }
};

void CheckBothArrays() {
    Runtime runtime;
    Check(pal4::GameDataAiPropertyCapacity::Reserve(0x1000, 5, runtime) ==
              0x7010 &&
              runtime.words[0x1060] == 0x6000 &&
              runtime.words[0x1064] == 0x6000 + 2 * 68 &&
              runtime.words[0x1068] == 0x6000 + 5 * 68 &&
              runtime.words[0x1008] == 0x7000 &&
              runtime.words[0x100c] == 0x7010 &&
              runtime.words[0x1010] == 0x7028,
          "68-byte values and 8-byte keys both grow to requested capacity");
    Check(runtime.records[0x6000][16] == 100 &&
              runtime.records[0x6044][16] == 101 &&
              runtime.keys[0x7000].record_index == 0 &&
              runtime.keys[0x7008].record_index == 1 &&
              runtime.calls.front() == "allocate 340",
          "both live property records and key mappings survive reallocation");
}

void CheckIndependentGrowth() {
    Runtime records_only;
    records_only.words[0x1010] = 0x5000 + 8 * 8;
    Check(pal4::GameDataAiPropertyCapacity::Reserve(
              0x1000, 5, records_only) == 8 &&
              records_only.words[0x1060] == 0x6000 &&
              records_only.words[0x1008] == 0x5000 &&
              records_only.calls.front() == "allocate 340",
          "record-only growth returns existing key capacity count");

    Runtime keys_only;
    keys_only.words[0x1068] = 0x4000 + 8 * 68;
    keys_only.allocations = {0x7000};
    Check(pal4::GameDataAiPropertyCapacity::Reserve(
              0x1000, 5, keys_only) == 0x7010 &&
              keys_only.words[0x1060] == 0x4000 &&
              keys_only.calls.front() == "allocate 40",
          "key-only growth leaves property records in place");

    Runtime no_growth;
    no_growth.words[0x1068] = 0x4000 + 8 * 68;
    no_growth.words[0x1010] = 0x5000 + 8 * 8;
    Check(pal4::GameDataAiPropertyCapacity::Reserve(
              0x1000, 5, no_growth) == 8 &&
              no_growth.calls.empty(),
          "sufficient capacities do not mutate storage");
}

void CheckEmptyAndNullAllocation() {
    Runtime empty;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    empty.words[0x1008] = empty.words[0x100c] = empty.words[0x1010] = 0;
    Check(pal4::GameDataAiPropertyCapacity::Reserve(0x1000, 3, empty) ==
              0x7000 &&
              empty.words[0x1060] == empty.words[0x1064] &&
              empty.words[0x1068] == 0x6000 + 3 * 68 &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1010] == 0x7000 + 3 * 8,
          "null arrays allocate with zero used entries");

    Runtime failed;
    failed.allocations = {0, 0};
    Check(pal4::GameDataAiPropertyCapacity::Reserve(0x1000, 5, failed) ==
              16 &&
              failed.words[0x1060] == 0 &&
              failed.words[0x1064] == 2 * 68 &&
              failed.words[0x1068] == 5 * 68 &&
              failed.words[0x1008] == 0 &&
              failed.words[0x100c] == 16 &&
              failed.words[0x1010] == 40,
          "native null-allocation path frees old arrays and retains pointer math");
}

}  // namespace

int main() {
    try {
        CheckBothArrays();
        CheckIndependentGrowth();
        CheckEmptyAndNullAllocation();
        std::cout << "game_data_ai_property_capacity_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
