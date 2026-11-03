#pragma once

#include "pal4/game_data_upgrade_table.h"

#include <cstdint>
#include <vector>

namespace pal4 {

struct GameDataUpgradeIndexEntry {
    std::int32_t key = 0;
    std::uint32_t record_index = 0;
    bool operator==(const GameDataUpgradeIndexEntry&) const = default;
};

class GameDataUpgradeTableIndex final {
public:
    // IDA 0x4ec0f0: compare the UpgradeData key at +144.
    [[nodiscard]] static bool EqualKey(
        const GameDataUpgradeTableRecord& left,
        const GameDataUpgradeTableRecord& right);
    // IDA 0x4eb2c0: compact adjacent duplicate keys and rebuild the index.
    static void Rebuild(std::vector<GameDataUpgradeTableRecord>& records,
                        std::vector<GameDataUpgradeIndexEntry>& index);
};

}  // namespace pal4
