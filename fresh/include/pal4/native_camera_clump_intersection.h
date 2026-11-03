#pragma once
#include "pal4/camera_data.h"
#include <array>
#include <cstdint>
struct RpAtomic;
struct RpClump;
namespace pal4 {
// Original5EA170/5F9F30, using the current SDK camera frustum. Pixel words are
// FLOAT coordinates (RwV2d), not integer coordinates. Borrows a live SDK clump.
// Writes the original query at CameraData+12. last_atomic_index models92F558:
// it changes only on a strictly nearer hit, and survives misses/null queries.
[[nodiscard]] RpAtomic* IntersectNativeCameraClump(CameraDataState&,RpClump*,
    const std::array<float,2>& pixel,std::uint32_t& last_atomic_index);
}
