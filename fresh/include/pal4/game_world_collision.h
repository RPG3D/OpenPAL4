#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using GameWorldRayBits = std::array<std::uint32_t, 6>;
using GameWorldCollisionBits = std::array<std::uint32_t, 6>;

struct GameWorldClumpCollisionContext final {
    GameWorldRayBits ray{};
    std::uint32_t intersection_type = 1u;
    GameWorldCollisionBits* collision = nullptr;
};

class IGameWorldCollisionRuntime {
public:
    virtual ~IGameWorldCollisionRuntime() = default;
    virtual void ForAllWorldIntersections(
        std::uint32_t world, const GameWorldRayBits& ray,
        std::uint32_t intersection_type,
        GameWorldCollisionBits& collision) = 0;
    virtual void ForAllClumpAtomics(
        std::uint32_t clump, GameWorldClumpCollisionContext& context) = 0;
    virtual void ForAllAtomicIntersections(
        std::uint32_t atomic, GameWorldClumpCollisionContext& context) = 0;
};

class GameWorldCollision final {
public:
    static constexpr std::uint32_t kMaxDistanceBits = 0x7f7fffffu;

    [[nodiscard]] static std::uint32_t Process(
        const GameWorldRayBits* ray, std::uint32_t world,
        GameWorldCollisionBits& collision,
        IGameWorldCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnIntersection(
        std::uint32_t triangle, std::uint32_t triangle_id,
        std::uint32_t distance_bits, GameWorldCollisionBits& collision);
    [[nodiscard]] static std::uint32_t ProcessClump(
        const GameWorldRayBits* ray, std::uint32_t clump,
        GameWorldCollisionBits& collision,
        IGameWorldCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t ProcessAtomic(
        const GameWorldRayBits* ray, std::uint32_t atomic,
        GameWorldCollisionBits& collision,
        IGameWorldCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnAtomic(
        std::uint32_t atomic, GameWorldClumpCollisionContext& context,
        IGameWorldCollisionRuntime& runtime);
    [[nodiscard]] static std::uint32_t OnAtomicIntersection(
        std::uint32_t triangle, std::uint32_t triangle_id,
        std::uint32_t distance_bits,
        GameWorldClumpCollisionContext& context);
};

}  // namespace pal4
