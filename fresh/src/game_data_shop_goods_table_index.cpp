#include "pal4/game_data_shop_goods_table_index.h"

#include <algorithm>
#include <cstddef>

namespace pal4 {

bool GameDataShopGoodsTableIndex::KeysEqual(
    const GameDataShopGoodsRecord& left,
    const GameDataShopGoodsRecord& right) noexcept {
    return left[0] == right[0];
}

void GameDataShopGoodsTableIndex::Rebuild(
    std::vector<GameDataShopGoodsRecord>& records,
    std::vector<GameDataShopGoodsIndexEntry>& index,
    IGameDataShopGoodsTableIndexRuntime& runtime) {
    std::size_t retained = 0;
    for (std::size_t source = 0; source < records.size(); ++source) {
        if (retained && KeysEqual(records[source], records[retained - 1]))
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
        index.push_back({static_cast<std::int32_t>(records[i][0]), i});
    std::stable_sort(index.begin(), index.end(),
                     [](const auto& left, const auto& right) {
                         return left.key < right.key;
                     });
}

}  // namespace pal4
