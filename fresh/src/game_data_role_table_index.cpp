#include "pal4/game_data_role_table_index.h"

#include <algorithm>
#include <cstdint>
#include <cstddef>

namespace pal4 {

bool GameDataRoleTableIndex::EqualKey(
    const GameDataRoleTableRecord& left,
    const GameDataRoleTableRecord& right) {
    return left[57] == right[57];
}

void GameDataRoleTableIndex::Rebuild(
    std::vector<GameDataRoleTableRecord>& records,
    std::vector<GameDataRoleIndexEntry>& index,
    IGameDataRoleTableIndexRuntime& runtime) {
    std::size_t retained = 0;
    for (std::size_t source = 0; source < records.size(); ++source) {
        if (retained && EqualKey(records[source], records[retained - 1]))
            continue;
        if (retained != source)
            runtime.AssignRecord(records[retained], records[source]);
        ++retained;
    }
    for (std::size_t i = retained; i < records.size(); ++i)
        runtime.DestroyRecord(records[i]);
    records.resize(retained);

    index.clear();
    index.reserve(records.size());
    for (std::uint32_t i = 0; i < records.size(); ++i)
        index.push_back({static_cast<std::int32_t>(records[i][57]), i});
    std::stable_sort(index.begin(), index.end(),
                     [](const auto& left, const auto& right) {
                         return left.key < right.key;
                     });
}

}  // namespace pal4
