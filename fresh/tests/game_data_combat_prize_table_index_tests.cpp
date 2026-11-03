#include "pal4/game_data_combat_prize_table_index.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

pal4::GameDataCombatPrizeRecord Record(std::uint32_t key,
                                       std::uint32_t marker) {
    pal4::GameDataCombatPrizeRecord record{};
    record[0] = key;
    record[31] = marker;
    return record;
}

void CheckDuplicatesAndSort() {
    std::vector<pal4::GameDataCombatPrizeRecord> records{
        Record(5, 1), Record(5, 2), Record(0xffffffffu, 3),
        Record(5, 4), Record(5, 5), Record(2, 6)};
    std::vector<pal4::GameDataCombatPrizeIndexEntry> index{{99, 99}};
    pal4::GameDataCombatPrizeTableIndex::Rebuild(records, index);
    Check(records.size() == 4 && records[0][31] == 1 &&
              records[1][31] == 3 && records[2][31] == 4 &&
              records[3][31] == 6,
          "adjacent duplicate runs retain first record and later recurrence");
    Check(index == std::vector<pal4::GameDataCombatPrizeIndexEntry>{
                       {-1, 1}, {2, 3}, {5, 0}, {5, 2}},
          "signed sorted keys retain original compacted indices");
}

void CheckLongAndEmpty() {
    std::vector<pal4::GameDataCombatPrizeRecord> records;
    for (std::uint32_t i = 0; i < 25; ++i)
        records.push_back(Record(24 - i, i + 100));
    std::vector<pal4::GameDataCombatPrizeIndexEntry> index;
    pal4::GameDataCombatPrizeTableIndex::Rebuild(records, index);
    Check(index.size() == 25 &&
              index.front() == pal4::GameDataCombatPrizeIndexEntry{0, 24} &&
              index.back() == pal4::GameDataCombatPrizeIndexEntry{24, 0} &&
              records.front()[31] == 100,
          "large index sorts without reordering records");
    records.clear();
    pal4::GameDataCombatPrizeTableIndex::Rebuild(records, index);
    Check(index.empty(), "empty record range clears prior index");
}

}  // namespace

int main() {
    try {
        CheckDuplicatesAndSort();
        CheckLongAndEmpty();
        std::cout << "game_data_combat_prize_table_index_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
