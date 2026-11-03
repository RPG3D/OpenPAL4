#pragma once

#include "pal4/game_data_shop_goods_table.h"

#include <cstdint>
#include <vector>

namespace pal4 {

struct GameDataShopGoodsIndexEntry {
    std::int32_t key;
    std::uint32_t record_index;
};

class IGameDataShopGoodsTableIndexRuntime {
public:
    virtual ~IGameDataShopGoodsTableIndexRuntime() = default;
    virtual void AssignRecord(GameDataShopGoodsRecord& destination,
                              const GameDataShopGoodsRecord& source) = 0;
    virtual void DestroyRecord(GameDataShopGoodsRecord& record) = 0;
};

class GameDataShopGoodsTableIndex final {
public:
    // IDA 0x4f0810: compare the first DWORD of two Good records.
    [[nodiscard]] static bool KeysEqual(
        const GameDataShopGoodsRecord& left,
        const GameDataShopGoodsRecord& right) noexcept;
    // IDA 0x4f03f0: compact adjacent duplicates, then rebuild signed index.
    static void Rebuild(
        std::vector<GameDataShopGoodsRecord>& records,
        std::vector<GameDataShopGoodsIndexEntry>& index,
        IGameDataShopGoodsTableIndexRuntime& runtime);
};

}  // namespace pal4
