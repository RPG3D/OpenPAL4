#include "pal4/game_data_shop_goods_index.h"

#include <algorithm>

namespace pal4 {

std::uint32_t GameDataShopGoodsIndex::FindByHash(
    std::uint32_t hash) const {
    const auto found = std::lower_bound(
        entries_.begin(), entries_.end(), hash,
        [](const auto& entry, std::uint32_t key) {
            return entry.hash < key;
        });
    if (found == entries_.end() || found->hash != hash) return 0;
    return record_base_ + kRecordBytes * found->record_index;
}

std::uint32_t GameDataShopGoodsIndex::FindByText(
    const std::uint8_t* data, std::uint32_t length,
    LegacyCrc32& crc) const {
    return FindByHash(crc.ComputeAndStore(data, length));
}

}  // namespace pal4
