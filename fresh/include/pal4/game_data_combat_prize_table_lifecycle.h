#pragma once

#include "pal4/game_data_effect_table_runtime.h"

#include <cstdint>

namespace pal4 {

class GameDataCombatPrizeTableLifecycle final {
public:
    static constexpr std::uint32_t kSize = 140;
    static constexpr std::uint32_t kVtable = 0x84468c;

    // IDA 0x4eb670, 0x4eb740, 0x4eb840.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t name, IGameDataEffectTableRuntime& runtime);
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataEffectTableRuntime& runtime);
    [[nodiscard]] static std::uint32_t ClearLoadedRecords(
        std::uint32_t object, IGameDataEffectTableRuntime& runtime);
};

}  // namespace pal4
