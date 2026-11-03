#include "pal4/camera_clump_intersection.h"

#include <bit>

namespace pal4 {

std::uint32_t CameraClumpIntersection::FindNearestAtomic(
    const std::uint32_t record, const std::uint32_t camera,
    const std::uint32_t clump, const std::uint32_t pixel,
    ICameraClumpIntersectionRuntime& runtime) {
    if (clump == 0u) return 0u;
    runtime.WriteWord(record + 24u, 1u);
    runtime.CalculatePixelRay(camera, record, pixel);
    runtime.WriteWord(record + 28u, 0u);
    runtime.WriteWord(record + 32u, 0x7f7fffffu);
    runtime.WriteWord(record + 36u, 0u);
    (void)runtime.ForEachAtomic(clump, record);
    return runtime.ReadWord(record + 28u);
}

std::uint32_t CameraClumpIntersection::ProcessAtomic(
    const std::uint32_t atomic, const std::uint32_t record,
    ICameraClumpIntersectionRuntime& runtime) {
    const auto previous = std::bit_cast<float>(
        runtime.ReadWord(record + 32u));
    (void)runtime.ForEachIntersection(atomic, record, record + 32u);
    const auto nearest = std::bit_cast<float>(
        runtime.ReadWord(record + 32u));
    if (nearest < previous) {
        const auto index = runtime.ReadWord(record + 36u);
        runtime.WriteWord(record + 28u, atomic);
        runtime.WriteWord(0x92f558u, index);
    }
    const auto index = runtime.ReadWord(record + 36u);
    runtime.WriteWord(record + 36u, index + 1u);
    return atomic;
}

std::uint32_t CameraClumpIntersection::UpdateDistance(
    const std::uint32_t, const std::uint32_t triangle,
    const float distance, const std::uint32_t nearest_distance,
    ICameraClumpIntersectionRuntime& runtime) {
    if (distance < std::bit_cast<float>(runtime.ReadWord(nearest_distance)))
        runtime.WriteWord(nearest_distance, std::bit_cast<std::uint32_t>(distance));
    return triangle;
}

}  // namespace pal4
