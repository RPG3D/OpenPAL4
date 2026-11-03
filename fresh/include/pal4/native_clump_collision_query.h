#pragma once

#include "pal4/collision_segment_geometry.h"

#include <cstdint>

struct RpClump;
struct RpAtomic;

namespace pal4 {

struct NativeClumpCollisionHit final {
    std::int32_t triangle = -1;
    RpAtomic* atomic = nullptr;
    float fraction = 0.0f;
    CollisionPoint3 point{};
};

// IDA 0x661a50, with the original collision plug-in doing triangle queries.
class NativeClumpCollisionQuery final {
public:
    [[nodiscard]] static bool Closest(const CollisionSegment& segment,
                                      RpClump* clump,
                                      NativeClumpCollisionHit& hit);
};

}  // namespace pal4
