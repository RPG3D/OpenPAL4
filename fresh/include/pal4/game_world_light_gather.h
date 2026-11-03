#pragma once

#include "pal4/game_world_object.h"

#include <cstdint>

namespace pal4 {

enum class WorldSectorCallback : std::uint8_t {
    GatherLight,
};

enum class WorldMeshCallback : std::uint8_t {
    SetTriStripMethod,
};

class IGameWorldLightGatherRuntime {
public:
    virtual ~IGameWorldLightGatherRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ForAllWorldSectors(
        std::uint32_t world, WorldSectorCallback callback,
        std::uint32_t user_data) = 0;
    virtual void ForAllSectorMeshes(std::uint32_t sector,
                                    WorldMeshCallback callback,
                                    std::uint32_t user_data) = 0;
};

class GameWorldLightGather final {
public:
    [[nodiscard]] static std::uint32_t Gather(
        const GameWorldObjectState& state,
        IGameWorldLightGatherRuntime& runtime);
    [[nodiscard]] static std::uint32_t GatherSectorMeshes(
        std::uint32_t sector, std::uint32_t user_data,
        IGameWorldLightGatherRuntime& runtime);
};

}  // namespace pal4
