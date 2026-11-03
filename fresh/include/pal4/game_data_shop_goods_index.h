#pragma once

#include "pal4/legacy_crc32.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace pal4 {

struct GameDataShopGoodsIndexEntry {
    std::uint32_t hash;
    std::uint32_t record_index;
};

class GameDataShopGoodsIndex final {
public:
    static constexpr std::uint32_t kRecordBytes = 56;

    GameDataShopGoodsIndex(std::uint32_t record_base,
                           std::vector<GameDataShopGoodsIndexEntry> entries)
        : record_base_(record_base), entries_(std::move(entries)) {}

    // IDA 0x4eeeb0: lower-bound lookup of an unsigned CRC key.
    [[nodiscard]] std::uint32_t FindByHash(std::uint32_t hash) const;
    // IDA 0x4eef20: hash a byte range, then look it up.
    [[nodiscard]] std::uint32_t FindByText(
        const std::uint8_t* data, std::uint32_t length,
        LegacyCrc32& crc) const;

private:
    std::uint32_t record_base_;
    std::vector<GameDataShopGoodsIndexEntry> entries_;
};

}  // namespace pal4
