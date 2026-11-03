#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using GameWorldPixelRay = std::array<float, 6>;
using GameWorldCollisionSample = std::array<std::uint32_t, 6>;
using GameWorldPointWords = std::array<std::uint32_t, 3>;

class IGameWorldNavmeshRuntime {
public:
    virtual ~IGameWorldNavmeshRuntime() = default;
    virtual void CalculatePixelRay(std::uint32_t camera, std::int32_t x,
                                   std::int32_t y,
                                   GameWorldPixelRay& ray) = 0;
    [[nodiscard]] virtual std::uint32_t NavClump() = 0;
    [[nodiscard]] virtual std::uint32_t Collide(
        const GameWorldPixelRay& ray, std::uint32_t clump,
        GameWorldCollisionSample& result) = 0;
};

class GameWorldNavmeshRuntimeStub final : public IGameWorldNavmeshRuntime {
public:
    void CalculatePixelRay(std::uint32_t, std::int32_t, std::int32_t,
                           GameWorldPixelRay&) override {}
    [[nodiscard]] std::uint32_t NavClump() override { return 0u; }
    [[nodiscard]] std::uint32_t Collide(
        const GameWorldPixelRay&, std::uint32_t,
        GameWorldCollisionSample&) override { return 0u; }
};

class GameWorldNavmeshProjector final {
public:
    static constexpr std::uint32_t kMissingCoordinate = 0x800000u;

    // IDA 0x51a250.
    [[nodiscard]] static std::uint32_t Project(
        std::uint32_t camera, std::int32_t x, std::int32_t y,
        GameWorldPointWords* output, IGameWorldNavmeshRuntime& runtime);
};

}  // namespace pal4
