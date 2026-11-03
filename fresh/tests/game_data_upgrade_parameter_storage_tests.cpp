#include "pal4/game_data_upgrade_parameter_table.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IGameDataUpgradeParameterStorageRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GameDataUpgradeParameterRecord> records;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x6000, 0x7000};

    Runtime() {
        words[0x1060] = 0x4000;
        words[0x1064] = 0x4000 + 4 * 92;
        words[0x1068] = 0x4000 + 5 * 92;
        words[0x1008] = 0x5000;
        words[0x100c] = 0x5020;
        words[0x1010] = 0x5000 + 5 * 8;
        for (std::uint32_t i = 0; i < 4; ++i) {
            pal4::GameDataUpgradeParameterRecord record{};
            record[0] = 100 + i;
            record[22] = std::vector<std::uint32_t>{9, 9, 0xffffffff, 3}[i];
            records[0x4000 + i * 92] = record;
            keys[0x5000 + i * 8] = {record[22], i};
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
    }
    pal4::GameDataUpgradeParameterRecord ReadRecord(
        std::uint32_t address) override {
        return records.at(address);
    }
    void WriteRecord(
        std::uint32_t address,
        const pal4::GameDataUpgradeParameterRecord& record) override {
        records[address] = record;
        calls.push_back("record " + std::to_string(address));
    }
    pal4::GameDataAiKeyEntry ReadKey(std::uint32_t address) override {
        return keys.at(address);
    }
    void WriteKey(std::uint32_t address,
                  const pal4::GameDataAiKeyEntry& entry) override {
        keys[address] = entry;
        calls.push_back("key " + std::to_string(address));
    }
    void AppendKey(std::uint32_t array,
                   const pal4::GameDataAiKeyEntry& entry) override {
        Check(array == 0x1004, "key array owner");
        const auto end = words[0x100c];
        keys[end] = entry;
        words[0x100c] = end + 8;
        calls.emplace_back("append key");
    }
    std::uint32_t Allocate(std::uint32_t bytes) override {
        calls.push_back("allocate " + std::to_string(bytes));
        Check(!allocations.empty(), "unexpected allocation");
        const auto result = allocations.front();
        allocations.erase(allocations.begin());
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
};

std::size_t At(const Runtime& runtime, const std::string& call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckSort() {
    Runtime runtime;
    pal4::GameDataUpgradeParameterStorage::Sort(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x4000 + 3 * 92 &&
              runtime.records[0x4000][0] == 100 &&
              runtime.records[0x4000 + 92][0] == 102 &&
              runtime.records[0x4000 + 184][0] == 103,
          "adjacent duplicate retains first complete 92-byte record");
    Check(runtime.words[0x100c] == 0x5018 &&
              runtime.keys[0x5000].key == 0xffffffff &&
              runtime.keys[0x5000].record_index == 1 &&
              runtime.keys[0x5008].key == 3 &&
              runtime.keys[0x5008].record_index == 2 &&
              runtime.keys[0x5010].key == 9 &&
              runtime.keys[0x5010].record_index == 0,
          "key map is rebuilt in signed order with compacted indices");
    Check(At(runtime, "record 16476") < At(runtime, "append key"),
          "record compaction precedes key-map reconstruction");

    Runtime empty;
    empty.words[0x1064] = empty.words[0x1060];
    pal4::GameDataUpgradeParameterStorage::Sort(0x1000, empty);
    Check(empty.words[0x1064] == 0x4000 &&
              empty.words[0x100c] == 0x5000,
          "empty record range resets key end");
}

void CheckTailKeyComparison() {
    pal4::GameDataUpgradeParameterRecord left{};
    pal4::GameDataUpgradeParameterRecord right{};
    left[0] = 1;
    right[0] = 2;
    left[22] = right[22] = 0xffffffff;
    Check(pal4::GameDataUpgradeParameterStorage::EqualTailKeys(left, right),
          "only the tail key controls equality");
    right[22] = 0;
    Check(!pal4::GameDataUpgradeParameterStorage::EqualTailKeys(left, right),
          "different tail keys compare unequal");
}

void CheckReserve() {
    Runtime both;
    Check(pal4::GameDataUpgradeParameterStorage::Reserve(0x1000, 6, both) ==
              0x7020 &&
              both.words[0x1060] == 0x6000 &&
              both.words[0x1064] == 0x6000 + 4 * 92 &&
              both.words[0x1068] == 0x6000 + 6 * 92 &&
              both.words[0x1008] == 0x7000 &&
              both.words[0x100c] == 0x7020 &&
              both.words[0x1010] == 0x7030,
          "both arrays grow while retaining used counts");
    Check(both.records[0x6000][0] == 100 &&
              both.records[0x6000 + 3 * 92][22] == 3 &&
              both.keys[0x7000 + 3 * 8].record_index == 3 &&
              both.calls.front() == "allocate 552" &&
              At(both, "free 16384") < At(both, "allocate 48") &&
              At(both, "free 20480") < both.calls.size(),
          "live 92-byte records and complete keys copy before free");

    Runtime records_only;
    records_only.words[0x1010] = 0x5000 + 8 * 8;
    Check(pal4::GameDataUpgradeParameterStorage::Reserve(
              0x1000, 6, records_only) == 8 &&
              records_only.words[0x1060] == 0x6000 &&
              records_only.words[0x1008] == 0x5000,
          "record-only growth returns existing key capacity");

    Runtime keys_only;
    keys_only.words[0x1068] = 0x4000 + 8 * 92;
    keys_only.allocations = {0x7000};
    Check(pal4::GameDataUpgradeParameterStorage::Reserve(
              0x1000, 6, keys_only) == 0x7020 &&
              keys_only.words[0x1060] == 0x4000 &&
              keys_only.calls.front() == "allocate 48",
          "key-only growth preserves record storage");

    Runtime enough;
    enough.words[0x1068] = 0x4000 + 8 * 92;
    enough.words[0x1010] = 0x5000 + 8 * 8;
    Check(pal4::GameDataUpgradeParameterStorage::Reserve(
              0x1000, 6, enough) == 8 && enough.calls.empty(),
          "sufficient capacities require no writes");
}

void CheckAllocationEdgeCases() {
    Runtime empty;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    empty.words[0x1008] = empty.words[0x100c] = empty.words[0x1010] = 0;
    Check(pal4::GameDataUpgradeParameterStorage::Reserve(0x1000, 3, empty) ==
              0x7000 &&
              empty.words[0x1060] == empty.words[0x1064] &&
              empty.words[0x1068] == 0x6000 + 3 * 92 &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1010] == 0x7000 + 3 * 8,
          "null arrays allocate new capacities with zero used elements");

    Runtime failed;
    failed.allocations = {0, 0};
    Check(pal4::GameDataUpgradeParameterStorage::Reserve(0x1000, 6, failed) ==
              32 &&
              failed.words[0x1060] == 0 &&
              failed.words[0x1064] == 4 * 92 &&
              failed.words[0x1008] == 0 &&
              failed.words[0x100c] == 32,
          "native null allocation still updates both array triplets");
}

}  // namespace

int main() {
    try {
        CheckSort();
        CheckTailKeyComparison();
        CheckReserve();
        CheckAllocationEdgeCases();
        std::cout << "game_data_upgrade_parameter_storage_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
