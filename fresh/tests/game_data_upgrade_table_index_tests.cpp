#include "pal4/game_data_upgrade_table_index.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

pal4::GameDataUpgradeTableRecord Record(std::uint32_t key,
                                        std::uint32_t marker) {
    pal4::GameDataUpgradeTableRecord record{};
    record[0] = marker;
    record[36] = key;
    return record;
}

void CheckAdjacentDuplicates() {
    Check(pal4::GameDataUpgradeTableIndex::EqualKey(
              Record(7, 1), Record(7, 2)) &&
              !pal4::GameDataUpgradeTableIndex::EqualKey(
                  Record(7, 1), Record(8, 1)),
          "UpgradeData comparator reads only the key at +144");
    std::vector<pal4::GameDataUpgradeTableRecord> records{
        Record(7, 1), Record(7, 2), Record(2, 3), Record(7, 4),
        Record(7, 5), Record(0xffffffffu, 6)};
    std::vector<pal4::GameDataUpgradeIndexEntry> index{{99, 99}};
    pal4::GameDataUpgradeTableIndex::Rebuild(records, index);
    Check(records.size() == 4 && records[0][0] == 1 &&
              records[1][0] == 3 && records[2][0] == 4 &&
              records[3][0] == 6,
          "only adjacent duplicates are removed; first record is retained");
    Check(index == std::vector<pal4::GameDataUpgradeIndexEntry>{
                       {-1, 3}, {2, 1}, {7, 0}, {7, 2}},
          "signed key order preserves record positions and repeated later keys");
}

void CheckLongAndEmpty() {
    std::vector<pal4::GameDataUpgradeTableRecord> records;
    for (std::uint32_t i = 0; i < 24; ++i)
        records.push_back(Record(23 - i, i + 100));
    std::vector<pal4::GameDataUpgradeIndexEntry> index;
    pal4::GameDataUpgradeTableIndex::Rebuild(records, index);
    Check(index.size() == 24 && index.front() ==
              pal4::GameDataUpgradeIndexEntry{0, 23} &&
              index.back() == pal4::GameDataUpgradeIndexEntry{23, 0} &&
              records.front()[0] == 100,
          "large index sorts keys without reordering records");
    records.clear();
    pal4::GameDataUpgradeTableIndex::Rebuild(records, index);
    Check(index.empty(), "empty record range clears old index");
}

}  // namespace

int main() {
    try {
        CheckAdjacentDuplicates();
        CheckLongAndEmpty();
        std::cout << "game_data_upgrade_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
