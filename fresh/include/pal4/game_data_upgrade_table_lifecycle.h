#pragma once

#include "pal4/game_data_effect_table_runtime.h"

#include <cstdint>

namespace pal4 {

class GameDataUpgradeTableLifecycle final {
public:
    static constexpr std::uint32_t kSize = 140;
    static constexpr std::uint32_t kVtable = 0x84466c;

    // IDA 0x4eac30: allocate and initialize an UpgradeData Effect table.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t name, IGameDataEffectTableRuntime& runtime);
    // IDA 0x4ead00: release the three embedded arrays and Effect base.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataEffectTableRuntime& runtime);
    // IDA 0x4eae00: clear loaded data and return the previous array end.
    [[nodiscard]] static std::uint32_t ClearLoadedRecords(
        std::uint32_t object, IGameDataEffectTableRuntime& runtime);
};

}  // namespace pal4
