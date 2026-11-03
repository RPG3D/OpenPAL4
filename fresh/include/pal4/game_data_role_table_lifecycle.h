#pragma once

#include "pal4/game_data_effect_table_runtime.h"

#include <cstdint>

namespace pal4 {

using IGameDataRoleTableRuntime = IGameDataEffectTableRuntime;

class GameDataRoleTableLifecycle final {
public:
    static constexpr std::uint32_t kSize = 140;
    static constexpr std::uint32_t kVtable = 0x84464c;

    // IDA 0x4e9fc0.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t name, IGameDataRoleTableRuntime& runtime);
    // IDA 0x4ea090.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t object, IGameDataRoleTableRuntime& runtime);
    // IDA 0x4ea210: release an embedded 320-byte record array.
    [[nodiscard]] static std::uint32_t DestroyRecordArray(
        std::uint32_t array, IGameDataRoleTableRuntime& runtime);
    // IDA 0x4ea250: clear loaded records while retaining capacity.
    static void ClearLoadedRecords(
        std::uint32_t object, IGameDataRoleTableRuntime& runtime);
    // IDA 0x4eaa80: destroy a half-open range of 320-byte records.
    static void DestroyRecordRange(
        std::uint32_t begin, std::uint32_t end,
        IGameDataRoleTableRuntime& runtime);
};

}  // namespace pal4
