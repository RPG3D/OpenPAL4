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

struct Runtime final : pal4::IGameDataAiPropertySortRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, pal4::GameDataAiPropertyRecord> records;
    std::map<std::uint32_t, pal4::GameDataAiKeyEntry> keys;
    std::vector<std::string> calls;

    explicit Runtime(const std::vector<std::uint32_t>& source_keys,
                     bool null_storage = false) {
        words[0x1008] = 0x2000;
        words[0x100c] = 0x2020;
        words[0x1060] = null_storage ? 0 : 0x4000;
        words[0x1064] = words[0x1060] +
                        static_cast<std::uint32_t>(source_keys.size()) * 68;
        for (std::size_t i = 0; i < source_keys.size(); ++i) {
            pal4::GameDataAiPropertyRecord record{};
            record[0] = source_keys[i];
            record[16] = static_cast<std::uint32_t>(i + 100);
            records[0x4000 + static_cast<std::uint32_t>(i) * 68] = record;
        }
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("word " + std::to_string(address));
        if (address == 0x100c) keys.clear();
    }
    pal4::GameDataAiPropertyRecord ReadRecord(
        std::uint32_t address) override {
        return records.at(address);
    }
    void WriteRecord(
        std::uint32_t address,
        const pal4::GameDataAiPropertyRecord& record) override {
        calls.push_back("copy " + std::to_string(address));
        records[address] = record;
    }
    void AppendKey(std::uint32_t array,
                   const pal4::GameDataAiKeyEntry& entry) override {
        Check(array == 0x1004, "property key collection address");
        keys[words[0x100c]] = entry;
        words[0x100c] += 8;
    }
    void WriteKey(std::uint32_t address,
                  const pal4::GameDataAiKeyEntry& entry) override {
        keys[address] = entry;
    }
    pal4::GameDataAiKeyEntry KeyAt(std::uint32_t index) const {
        return keys.at(0x2000 + index * 8);
    }
};

void CheckEmptyAndDuplicates() {
    Runtime empty({}, true);
    pal4::GameDataAiPropertySorter::Sort(0x1000, empty);
    Check(empty.words[0x1064] == 0 && empty.words[0x100c] == 0x2000 &&
              empty.keys.empty(),
          "empty property arrays reset both ends");

    Runtime duplicates({3, 3, 1, 1, 2});
    pal4::GameDataAiPropertySorter::Sort(0x1000, duplicates);
    Check(duplicates.words[0x1064] == 0x4000 + 3 * 68 &&
              duplicates.records[0x4000][16] == 100 &&
              duplicates.records[0x4044][16] == 102 &&
              duplicates.records[0x4088][16] == 104 &&
              duplicates.KeyAt(0).key == 1 &&
              duplicates.KeyAt(0).record_index == 1 &&
              duplicates.KeyAt(1).key == 2 &&
              duplicates.KeyAt(1).record_index == 2 &&
              duplicates.KeyAt(2).key == 3 &&
              duplicates.KeyAt(2).record_index == 0,
          "68-byte compaction retains first records and remaps sorted keys");
}

void CheckSignedAndLarge() {
    Runtime non_adjacent({2, 1, 2});
    pal4::GameDataAiPropertySorter::Sort(0x1000, non_adjacent);
    Check(non_adjacent.words[0x1064] == 0x4000 + 3 * 68 &&
              non_adjacent.KeyAt(1).record_index == 0 &&
              non_adjacent.KeyAt(2).record_index == 2,
          "non-adjacent equal keys remain separate");

    Runtime signed_keys({0u, 0xffffffffu, 2u, 0x80000000u});
    pal4::GameDataAiPropertySorter::Sort(0x1000, signed_keys);
    Check(signed_keys.KeyAt(0).key == 0x80000000u &&
              signed_keys.KeyAt(1).key == 0xffffffffu &&
              signed_keys.KeyAt(2).key == 0u &&
              signed_keys.KeyAt(3).key == 2u,
          "property sort compares keys as signed DWORDs");

    std::vector<std::uint32_t> descending;
    for (std::uint32_t value = 17; value != 0; --value)
        descending.push_back(value);
    Runtime large(descending);
    pal4::GameDataAiPropertySorter::Sort(0x1000, large);
    Check(large.words[0x1064] == 0x4000 + 17 * 68 &&
              large.words[0x100c] == 0x2000 + 17 * 8,
          "large property sort retains array lengths");
    for (std::uint32_t index = 0; index < 17; ++index)
        Check(large.KeyAt(index).key == index + 1 &&
                  large.KeyAt(index).record_index == 16 - index,
              "large property sort preserves key-to-record mapping");
}

}  // namespace

int main() {
    try {
        CheckEmptyAndDuplicates();
        CheckSignedAndLarge();
        std::cout << "game_data_ai_property_sorter_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
