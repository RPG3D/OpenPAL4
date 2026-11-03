#include "pal4/pal_movement_collision_router.h"

namespace pal4 {

std::uint32_t PalMovementCollisionRuntimeStub::CheckFirst(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t&, std::uint32_t,
    std::uint32_t) { return 0u; }
std::uint32_t PalMovementCollisionRuntimeStub::CheckSecond(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t&, std::uint32_t,
    std::uint32_t) { return 0u; }
std::uint32_t PalMovementCollisionRuntimeStub::CheckThird(
    std::uint32_t, std::uint32_t, std::uint32_t,
    std::uint32_t, std::uint32_t, std::uint32_t&,
    std::uint32_t, std::uint32_t) { return 0u; }

std::uint32_t PalMovementCollisionRouter::CheckStages(
    std::uint32_t object, std::uint32_t enable_first,
    std::uint32_t enable_second, std::uint32_t enable_third,
    std::uint32_t first, std::uint32_t second,
    std::uint32_t third, std::uint32_t extra,
    IPalMovementCollisionRuntime& runtime) {
    std::uint32_t result = 0u;
    std::uint32_t hit = 0u;
    if (enable_first != 0u)
        result = runtime.CheckFirst(
            object, first, second, third, hit, 0u, 0u);
    if (enable_second != 0u && hit == 0u)
        result = runtime.CheckSecond(
            object, first, second, third, hit, 0u, 0u);
    if (enable_third != 0u && hit == 0u)
        result = runtime.CheckThird(
            object, first, second, extra, third, hit, 0u, 0u);
    return result;
}

}  // namespace pal4
