#pragma once

#include "pal4/game_data_combat_prize_table.h"

#include <cstdint>
#include <vector>

namespace pal4 {

struct GameDataCombatPrizeIndexEntry {
    std::int32_t key = 0;
    std::uint32_t record_index = 0;
    bool operator==(const GameDataCombatPrizeIndexEntry&) const = default;
};

class GameDataCombatPrizeTableIndex final {
public:
    // IDA 0x4ebce0: compact adjacent duplicate keys and rebuild the index.
    static void Rebuild(std::vector<GameDataCombatPrizeRecord>& records,
                        std::vector<GameDataCombatPrizeIndexEntry>& index);
};

}  // namespace pal4
