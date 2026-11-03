#include "pal4/game_data_combat_prize_table_index.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

namespace pal4 {

void GameDataCombatPrizeTableIndex::Rebuild(
    std::vector<GameDataCombatPrizeRecord>& records,
    std::vector<GameDataCombatPrizeIndexEntry>& index) {
    std::size_t retained = 0;
    for (std::size_t source = 0; source < records.size(); ++source) {
        if (retained && records[source][0] == records[retained - 1][0])
            continue;
        if (retained != source) records[retained] = records[source];
        ++retained;
    }
    records.resize(retained);
    index.clear();
    index.reserve(records.size());
    for (std::uint32_t i = 0; i < records.size(); ++i)
        index.push_back({static_cast<std::int32_t>(records[i][0]), i});
    std::stable_sort(index.begin(), index.end(),
                     [](const auto& left, const auto& right) {
                         return left.key < right.key;
                     });
}

}  // namespace pal4
