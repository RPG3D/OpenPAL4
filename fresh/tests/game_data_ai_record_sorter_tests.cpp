#include "pal4/game_data_ai_record_sorter.h"

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

struct Runtime final : pal4::IGameDataAiRecordSortRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GameDataAiRecord> records;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;

    explicit Runtime(const std::vector<std::uint32_t>& source_keys,
                     bool null_storage = false) {
        words[0x1008] = 0x2000;
        words[0x100c] = 0x2020;
        words[0x1060] = null_storage ? 0 : 0x4000;
        words[0x1064] = words[0x1060] +
                        static_cast<std::uint32_t>(source_keys.size()) * 56;
        for (std::size_t i = 0; i < source_keys.size(); ++i) {
            pal4::GameDataAiRecord record{};
            record[0] = source_keys[i];
            record[1] = static_cast<std::uint32_t>(i + 100);
            records[0x4000 + static_cast<std::uint32_t>(i) * 56] = record;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address) + " " +
                        std::to_string(value));
        if (address == 0x100c) keys.clear();
    }
    pal4::GameDataAiRecord ReadRecord(std::uint32_t address) override {
        calls.push_back("read record " + std::to_string(address));
        return records.at(address);
    }
    void WriteRecord(std::uint32_t address,
                     const pal4::GameDataAiRecord& record) override {
        calls.push_back("write record " + std::to_string(address));
        records[address] = record;
    }
    void AppendKey(std::uint32_t array,
                   const pal4::GameDataAiKeyEntry& entry) override {
        Check(array == 0x1004, "key collection address");
        calls.push_back("append " + std::to_string(entry.record_index));
        keys[words[0x100c]] = entry;
        words[0x100c] += 8;
    }
    void WriteKey(std::uint32_t address,
                  const pal4::GameDataAiKeyEntry& entry) override {
        calls.push_back("sort key " + std::to_string(address));
        keys[address] = entry;
    }
    pal4::GameDataAiKeyEntry KeyAt(std::uint32_t index) const {
        return keys.at(0x2000 + index * 8);
    }
};

void CheckEmptyAndSingle() {
    Runtime empty({}, true);
    pal4::GameDataAiRecordSorter::Sort(0x1000, empty);
    Check(empty.words[0x1064] == 0 && empty.words[0x100c] == 0x2000 &&
              empty.keys.empty() && empty.calls.size() == 2,
          "null empty array only resets both ends");

    Runtime single({7});
    pal4::GameDataAiRecordSorter::Sort(0x1000, single);
    Check(single.words[0x1064] == 0x4038 &&
              single.words[0x100c] == 0x2008 &&
              single.KeyAt(0).key == 7 &&
              single.KeyAt(0).record_index == 0 &&
              single.records[0x4000][1] == 100,
          "one record and one index are preserved");
}

void CheckAdjacentDuplicates() {
    Runtime runtime({3, 3, 1, 1, 2});
    pal4::GameDataAiRecordSorter::Sort(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x4000 + 3 * 56 &&
              runtime.records[0x4000][1] == 100 &&
              runtime.records[0x4038][1] == 102 &&
              runtime.records[0x4070][1] == 104,
          "each adjacent run keeps its first whole record");
    Check(runtime.words[0x100c] == 0x2000 + 3 * 8 &&
              runtime.KeyAt(0).key == 1 &&
              runtime.KeyAt(0).record_index == 1 &&
              runtime.KeyAt(1).key == 2 &&
              runtime.KeyAt(1).record_index == 2 &&
              runtime.KeyAt(2).key == 3 &&
              runtime.KeyAt(2).record_index == 0,
          "sorted keys still address the compacted record positions");
}

void CheckNonAdjacentAndSignedKeys() {
    Runtime non_adjacent({2, 1, 2});
    pal4::GameDataAiRecordSorter::Sort(0x1000, non_adjacent);
    Check(non_adjacent.words[0x1064] == 0x4000 + 3 * 56 &&
              non_adjacent.KeyAt(0).record_index == 1 &&
              non_adjacent.KeyAt(1).record_index == 0 &&
              non_adjacent.KeyAt(2).record_index == 2,
          "non-adjacent equal keys remain separate with short-sort order");

    Runtime signed_keys({0u, 0xffffffffu, 2u, 0x80000000u});
    pal4::GameDataAiRecordSorter::Sort(0x1000, signed_keys);
    Check(signed_keys.KeyAt(0).key == 0x80000000u &&
              signed_keys.KeyAt(1).key == 0xffffffffu &&
              signed_keys.KeyAt(2).key == 0u &&
              signed_keys.KeyAt(3).key == 2u,
          "keys use signed 32-bit order");
}

void CheckLargeSort() {
    std::vector<std::uint32_t> descending;
    for (std::uint32_t value = 17; value != 0; --value)
        descending.push_back(value);
    Runtime runtime(descending);
    pal4::GameDataAiRecordSorter::Sort(0x1000, runtime);
    Check(runtime.words[0x1064] == 0x4000 + 17 * 56 &&
              runtime.words[0x100c] == 0x2000 + 17 * 8,
          "large branch preserves record and key counts");
    for (std::uint32_t index = 0; index < 17; ++index)
        Check(runtime.KeyAt(index).key == index + 1 &&
                  runtime.KeyAt(index).record_index == 16 - index,
              "large branch sorts keys while retaining source indexes");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndSingle();
        CheckAdjacentDuplicates();
        CheckNonAdjacentAndSignedKeys();
        CheckLargeSort();
        std::cout << "game_data_ai_record_sorter_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
