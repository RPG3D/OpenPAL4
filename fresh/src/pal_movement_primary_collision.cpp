#include "pal4/pal_movement_primary_collision.h"

#include <cmath>

namespace pal4 {

std::uint32_t PalMovementPrimaryCollisionRuntimeStub::CheckObjectIntersection(
    std::uint32_t, float, float, std::uint32_t,
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t PalMovementPrimaryCollisionRuntimeStub::CheckHeightIntersection(
    std::uint32_t, float, float, std::array<float, 3>&,
    std::uint32_t, std::uint32_t) { return 0u; }
float PalMovementPrimaryCollisionRuntimeStub::ReadFloat(
    std::uint32_t) { return 0.0f; }

std::uint32_t PalMovementPrimaryChild::CheckIntersectionOptions(
    std::uint32_t object, std::uint32_t use_field70,
    std::uint32_t use_field6c, std::uint32_t first,
    float second, float third, std::uint32_t fourth,
    IPalMovementPrimaryCollisionRuntime& runtime) {
    const auto field6c = use_field6c != 0u ? object + 0x6cu : 0u;
    const auto field70 = use_field70 != 0u ? object + 0x70u : 0u;
    return runtime.CheckObjectIntersection(
        first, second, third, fourth, field70, field6c);
}

std::uint32_t PalMovementPrimaryChild::ValidateIntersectionHeight(
    std::uint32_t source, float second, float third,
    std::uint32_t check_height,
    IPalMovementPrimaryCollisionRuntime& runtime) {
    std::array<float, 3> output{};
    const auto result = runtime.CheckHeightIntersection(
        source, second, third, output, 0u, 0u);
    if (check_height == 0u) return result;
    const auto height_difference = std::fabs(
        static_cast<double>(runtime.ReadFloat(source + 4u)) -
        static_cast<double>(output[1]));
    return height_difference > 10.0 ? 0u : result;
}

}  // namespace pal4
