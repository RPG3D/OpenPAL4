#pragma once

#include "pal4/game_world_object.h"
#include "pal4/game_world_collision.h"

#include <array>
#include <cstdint>

namespace pal4 {

using GameWorldVectorBits = std::array<std::uint32_t, 3>;

class IGameWorldPixelRayRuntime : public IGameWorldCollisionRuntime {
public:
    virtual ~IGameWorldPixelRayRuntime() = default;
    virtual void CalculatePixelRay(std::uint32_t camera,
                                   GameWorldRayBits& ray,
                                   const std::array<std::uint32_t, 2>& pixel) = 0;
};

class GameWorldPixelRay final {
public:
    static constexpr std::uint32_t kDefaultCoordinateBits = 0x800000u;

    [[nodiscard]] static std::uint32_t Intersect(
        const GameWorldObjectState& state, std::uint32_t camera,
        std::uint32_t pixel_x, std::uint32_t pixel_y,
        GameWorldVectorBits* intersection,
        IGameWorldPixelRayRuntime& runtime);
};

}  // namespace pal4
