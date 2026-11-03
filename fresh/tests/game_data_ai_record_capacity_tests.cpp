#include "pal4/game_data_ai_record_capacity.h"

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

struct Runtime final : pal4::IGameDataAiRecordCapacityRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GameDataAiRecord> records;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> allocations{0x6000, 0x7000};

    Runtime() {
        words[0x1060] = 0x4000;
        words[0x1064] = 0x4000 + 2 * 56;
        words[0x1068] = 0x4000 + 3 * 56;
        words[0x1008] = 0x5000;
        words[0x100c] = 0x5010;
        words[0x1010] = 0x5000 + 3 * 8;
        for (std::uint32_t i = 0; i < 2; ++i) {
            pal4::GameDataAiRecord record{};
            record[0] = 10 + i;
            record[13] = 100 + i;
            records[0x4000 + i * 56] = record;
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
        const auto result = allocations.front();
        allocations.erase(allocations.begin());
        return result;
    }
    void Free(std::uint32_t address) override {
        calls.push_back("free " + std::to_string(address));
    }
    pal4::GameDataAiRecord ReadRecord(std::uint32_t address) override {
        return records.at(address);
    }
    void WriteRecord(std::uint32_t address,
                     const pal4::GameDataAiRecord& record) override {
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

std::size_t At(const Runtime& runtime, const std::string& value) {
    auto found = std::find(runtime.calls.begin(), runtime.calls.end(), value);
    Check(found != runtime.calls.end(), "expected call omitted");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CheckBothGrow() {
    Runtime runtime;
    Check(pal4::GameDataAiRecordCapacity::Reserve(0x1000, 5, runtime) ==
              0x7010 &&
              runtime.words[0x1060] == 0x6000 &&
              runtime.words[0x1064] == 0x6000 + 2 * 56 &&
              runtime.words[0x1068] == 0x6000 + 5 * 56 &&
              runtime.words[0x1008] == 0x7000 &&
              runtime.words[0x100c] == 0x7010 &&
              runtime.words[0x1010] == 0x7028,
          "both arrays grow to requested capacity and retain used lengths");
    Check(runtime.records[0x6000][0] == 10 &&
              runtime.records[0x6038][13] == 101 &&
              runtime.keys[0x7000].key == 10 &&
              runtime.keys[0x7008].record_index == 1,
          "all live entries are copied");
    Check(runtime.calls.front() == "allocate 280" &&
              At(runtime, "copy record 24632") < At(runtime, "free 16384") &&
              At(runtime, "free 16384") < At(runtime, "allocate 40") &&
              At(runtime, "copy key 28680") < At(runtime, "free 20480") &&
              runtime.calls.back() == "word 4108",
          "record allocation and copy precede key allocation and copy");
}

void CheckPartialAndNoGrowth() {
    Runtime records_only;
    records_only.words[0x1010] = 0x5000 + 8 * 8;
    Check(pal4::GameDataAiRecordCapacity::Reserve(
              0x1000, 5, records_only) == 8 &&
              records_only.words[0x1060] == 0x6000 &&
              records_only.words[0x1008] == 0x5000 &&
              records_only.calls.front() == "allocate 280" &&
              std::none_of(records_only.calls.begin(), records_only.calls.end(),
                           [](const std::string& call) {
                               return call == "allocate 40";
                           }),
          "record-only growth returns unchanged key capacity count");

    Runtime keys_only;
    keys_only.words[0x1068] = 0x4000 + 8 * 56;
    keys_only.allocations = {0x7000};
    Check(pal4::GameDataAiRecordCapacity::Reserve(
              0x1000, 5, keys_only) == 0x7010 &&
              keys_only.words[0x1060] == 0x4000 &&
              keys_only.calls.front() == "allocate 40",
          "key-only growth leaves record storage untouched");

    Runtime unchanged;
    unchanged.words[0x1068] = 0x4000 + 8 * 56;
    unchanged.words[0x1010] = 0x5000 + 8 * 8;
    Check(pal4::GameDataAiRecordCapacity::Reserve(
              0x1000, 5, unchanged) == 8 &&
              unchanged.calls.empty(),
          "sufficient capacities perform no writes or allocations");
}

void CheckNullStorageAndFailure() {
    Runtime empty;
    empty.words[0x1060] = empty.words[0x1064] = empty.words[0x1068] = 0;
    empty.words[0x1008] = empty.words[0x100c] = empty.words[0x1010] = 0;
    Check(pal4::GameDataAiRecordCapacity::Reserve(0x1000, 3, empty) ==
              0x7000 &&
              empty.words[0x1060] == empty.words[0x1064] &&
              empty.words[0x1068] == 0x6000 + 3 * 56 &&
              empty.words[0x1008] == empty.words[0x100c] &&
              empty.words[0x1010] == 0x7000 + 3 * 8 &&
              At(empty, "free 0") < At(empty, "allocate 24"),
          "null arrays allocate both capacities with zero used elements");

    Runtime failed;
    failed.allocations = {0, 0};
    Check(pal4::GameDataAiRecordCapacity::Reserve(0x1000, 5, failed) == 16 &&
              failed.words[0x1060] == 0 && failed.words[0x1064] == 112 &&
              failed.words[0x1008] == 0 && failed.words[0x100c] == 16 &&
              At(failed, "free 16384") < At(failed, "allocate 40") &&
              At(failed, "free 20480") < failed.calls.size(),
          "native allocation failure still frees old arrays and updates fields");
}

}  // namespace

int main() {
    try {
        CheckBothGrow();
        CheckPartialAndNoGrowth();
        CheckNullStorageAndFailure();
        std::cout << "game_data_ai_record_capacity_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
