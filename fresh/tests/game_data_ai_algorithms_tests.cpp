#include "pal4/game_data_ai_record_sorter.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckKeySorting() {
    using pal4::GameDataAiKeyAlgorithms;
    using pal4::GameDataAiKeyEntry;
    std::vector<GameDataAiKeyEntry> entries;
    for (std::uint32_t index = 0; index < 17; ++index)
        entries.push_back({5, index});
    const auto remaining =
        GameDataAiKeyAlgorithms::PartitionLarge(entries, 0, entries.size());
    Check(remaining <= 16 && entries.front().record_index == 16 &&
              entries.back().record_index == 0,
          "large partition returns a short remainder and exchanges ties");
    GameDataAiKeyAlgorithms::InsertionSort(entries, 0, entries.size());
    for (std::uint32_t index = 0; index < 17; ++index)
        Check(entries[index].record_index == 16 - index,
              "insertion pass retains equal-key partition order");

    std::vector<GameDataAiKeyEntry> mixed = {
        {3, 0}, {0xffffffffu, 1}, {2, 2}, {3, 3},
        {0x80000000u, 4}, {0, 5}};
    GameDataAiKeyAlgorithms::Sort(mixed);
    Check(mixed[0].record_index == 4 && mixed[1].record_index == 1 &&
              mixed[2].record_index == 5 && mixed[3].record_index == 2 &&
              mixed[4].record_index == 0 && mixed[5].record_index == 3,
          "short sort uses signed keys and retains equal-key order");

    for (std::size_t count : {17u, 18u, 31u, 32u, 64u, 129u}) {
        std::vector<GameDataAiKeyEntry> repeated;
        for (std::uint32_t index = 0; index < count; ++index)
            repeated.push_back({static_cast<std::uint32_t>(
                                    static_cast<std::int32_t>((index * 37) % 11) - 5),
                                index});
        GameDataAiKeyAlgorithms::Sort(repeated);
        std::set<std::uint32_t> indexes;
        for (std::size_t index = 0; index < count; ++index) {
            indexes.insert(repeated[index].record_index);
            if (index)
                Check(static_cast<std::int32_t>(repeated[index - 1].key) <=
                          static_cast<std::int32_t>(repeated[index].key),
                      "recursive partition leaves signed keys sorted");
        }
        Check(indexes.size() == count && *indexes.begin() == 0 &&
                  *indexes.rbegin() == count - 1,
              "recursive partition neither loses nor duplicates entries");
    }
}

void CheckSearchAndInsertion() {
    using pal4::GameDataAiKeyAlgorithms;
    std::vector<pal4::GameDataAiKeyEntry> entries = {
        {0xffffffffu, 1}, {2, 2}, {2, 3}, {5, 4}};
    Check(GameDataAiKeyAlgorithms::LowerBound(entries, 0xffffffffu) == 0 &&
              GameDataAiKeyAlgorithms::LowerBound(entries, 2) == 1 &&
              GameDataAiKeyAlgorithms::LowerBound(entries, 3) == 3 &&
              GameDataAiKeyAlgorithms::LowerBound(entries, 9) == 4,
          "lower bound finds first equal or insertion end");
    Check(GameDataAiKeyAlgorithms::LowerBound({}, 3) == 0,
          "empty lower bound returns zero");

    entries = {{1, 10}, {3, 30}, {5, 50}, {2, 20}};
    const auto inserted_index =
        GameDataAiKeyAlgorithms::InsertSorted(entries, 0, 3, entries[3]);
    Check(inserted_index == 20 && entries[1].key == 2 &&
              entries[1].record_index == 20 &&
              entries[2].key == 3 && entries[3].key == 5,
          "insertion moves complete key/index pairs");
    bool rejected = false;
    try {
        (void)GameDataAiKeyAlgorithms::InsertSorted(entries, 0,
                                                    entries.size(), {1, 0});
    } catch (const std::out_of_range&) {
        rejected = true;
    }
    Check(rejected, "invalid insertion position is rejected");
}

void CheckMedianAndPartition() {
    using pal4::GameDataAiKeyAlgorithms;
    using Entry = pal4::GameDataAiKeyEntry;
    Check(GameDataAiKeyAlgorithms::SelectMedian({1, 10}, {3, 30}, {2, 20})
                  .record_index == 20 &&
              GameDataAiKeyAlgorithms::SelectMedian({3, 30}, {1, 10}, {2, 20})
                  .record_index == 20 &&
              GameDataAiKeyAlgorithms::SelectMedian({3, 30}, {2, 20}, {1, 10})
                  .record_index == 20,
          "median selects the whole entry from every order");
    Check(GameDataAiKeyAlgorithms::SelectMedian({2, 10}, {2, 20}, {2, 30})
                  .record_index == 20,
          "equal median selects middle entry");
    std::vector<Entry> entries = {{4, 40}, {1, 10}, {3, 30},
                                  {2, 20}, {5, 50}};
    const auto split = GameDataAiKeyAlgorithms::Partition(entries, 0, 5, 3);
    for (std::size_t i = 0; i < split; ++i)
        Check(static_cast<std::int32_t>(entries[i].key) <= 3,
              "left partition key");
    for (std::size_t i = split; i < entries.size(); ++i)
        Check(static_cast<std::int32_t>(entries[i].key) >= 3,
              "right partition key");
    for (const auto& entry : entries)
        Check(entry.record_index == entry.key * 10,
              "partition preserves key/index association");
    std::vector<Entry> outside = {{1, 10}, {2, 20}, {3, 30}};
    Check(GameDataAiKeyAlgorithms::Partition(outside, 0, 3, 0) == 0 &&
              GameDataAiKeyAlgorithms::Partition(outside, 0, 3, 4) == 3,
          "out-of-range pivots stop at the respective bounds");
}

void CheckRecordCompaction() {
    std::array<pal4::GameDataAiRecord, 5> records{};
    for (std::size_t i = 0; i < records.size(); ++i) {
        records[i][0] = std::array<std::uint32_t, 5>{3, 3, 1, 3, 3}[i];
        records[i][13] = static_cast<std::uint32_t>(100 + i);
    }
    std::array<pal4::GameDataAiRecord, 5> output{};
    Check(pal4::GameDataAiRecordCompactor::Compact56(records, output) == 3 &&
              output[0][13] == 100 && output[1][13] == 102 &&
              output[2][13] == 103,
          "56-byte compaction retains first record in each adjacent run");
    Check(pal4::GameDataAiRecordCompactor::Compact56(records, records) == 3 &&
              records[2][13] == 103,
          "56-byte compaction supports in-place destination");

    std::array<pal4::GameDataAiPropertyRecord, 3> properties{};
    properties[0][0] = properties[1][0] = 7;
    properties[2][0] = 8;
    properties[0][16] = 10;
    properties[1][16] = 11;
    properties[2][16] = 12;
    std::array<pal4::GameDataAiPropertyRecord, 3> property_output{};
    Check(pal4::GameDataAiRecordCompactor::Compact68(properties,
                                                     property_output) == 2 &&
              property_output[0][16] == 10 && property_output[1][16] == 12,
          "68-byte compaction copies the complete first and last records");
    Check(pal4::GameDataAiRecordCompactor::Compact68({}, property_output) == 0,
          "empty property source leaves destination alone");
    bool rejected = false;
    try {
        const auto result = pal4::GameDataAiRecordCompactor::Compact68(
            properties, std::span<pal4::GameDataAiPropertyRecord>{});
        Check(result == 0, "unreachable short-destination result");
    } catch (const std::length_error&) {
        rejected = true;
    }
    Check(rejected, "short destination is rejected");
}

}  // namespace

int main() {
    try {
        CheckKeySorting();
        CheckSearchAndInsertion();
        CheckMedianAndPartition();
        CheckRecordCompaction();
        std::cout << "game_data_ai_algorithms_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
