#include "pal4/game_world_collision.h"

#include <bit>

namespace pal4 {
namespace {

float AsFloat(const std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

std::uint32_t AsBits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

void InitializeCollision(GameWorldCollisionBits& collision) {
    collision[0] = 0xffffffffu;
    collision[1] = 0u;
    collision[2] = GameWorldCollision::kMaxDistanceBits;
}

std::uint32_t FinishCollision(const GameWorldRayBits& ray,
                              GameWorldCollisionBits& collision) {
    const float distance = AsFloat(collision[2]);
    if (!(distance < AsFloat(GameWorldCollision::kMaxDistanceBits)))
        return 0u;

    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        const float origin = AsFloat(ray[axis]);
        const float end = AsFloat(ray[axis + 3u]);
        const float delta = end - origin;
        const float scaled = distance * delta;
        collision[axis + 3u] = AsBits(origin + scaled);
    }
    return 1u;
}

}  // namespace

std::uint32_t GameWorldCollision::Process(
    const GameWorldRayBits* ray, const std::uint32_t world,
    GameWorldCollisionBits& collision, IGameWorldCollisionRuntime& runtime) {
    if (ray == nullptr || world == 0u)
        return 0u;

    InitializeCollision(collision);
    runtime.ForAllWorldIntersections(world, *ray, 1u, collision);
    return FinishCollision(*ray, collision);
}

std::uint32_t GameWorldCollision::OnIntersection(
    const std::uint32_t triangle, const std::uint32_t triangle_id,
    const std::uint32_t distance_bits, GameWorldCollisionBits& collision) {
    if (AsFloat(distance_bits) < AsFloat(collision[2])) {
        collision[2] = distance_bits;
        collision[0] = triangle_id;
    }
    return triangle;
}

std::uint32_t GameWorldCollision::ProcessClump(
    const GameWorldRayBits* ray, const std::uint32_t clump,
    GameWorldCollisionBits& collision, IGameWorldCollisionRuntime& runtime) {
    if (ray == nullptr || clump == 0u)
        return 0u;
    InitializeCollision(collision);
    GameWorldClumpCollisionContext context{*ray, 1u, &collision};
    runtime.ForAllClumpAtomics(clump, context);
    return FinishCollision(*ray, collision);
}

std::uint32_t GameWorldCollision::ProcessAtomic(
    const GameWorldRayBits* ray, const std::uint32_t atomic,
    GameWorldCollisionBits& collision, IGameWorldCollisionRuntime& runtime) {
    if (ray == nullptr || atomic == 0u)
        return 0u;
    InitializeCollision(collision);
    GameWorldClumpCollisionContext context{*ray, 1u, &collision};
    static_cast<void>(OnAtomic(atomic, context, runtime));
    return FinishCollision(*ray, collision);
}

std::uint32_t GameWorldCollision::OnAtomic(
    const std::uint32_t atomic, GameWorldClumpCollisionContext& context,
    IGameWorldCollisionRuntime& runtime) {
    auto& collision = *context.collision;
    const auto previous_distance = AsFloat(collision[2]);
    runtime.ForAllAtomicIntersections(atomic, context);
    if (AsFloat(collision[2]) < previous_distance)
        collision[1] = atomic;
    return atomic;
}

std::uint32_t GameWorldCollision::OnAtomicIntersection(
    const std::uint32_t triangle, const std::uint32_t triangle_id,
    const std::uint32_t distance_bits,
    GameWorldClumpCollisionContext& context) {
    return OnIntersection(triangle, triangle_id, distance_bits,
                          *context.collision);
}

}  // namespace pal4
