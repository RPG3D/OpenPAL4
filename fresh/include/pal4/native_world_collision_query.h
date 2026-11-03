#pragma once

#include "pal4/collision_segment_geometry.h"

struct RpWorld;

namespace pal4 {

// SDK traversal for the shared 0x5fea10 segment intersection algorithm.
class NativeWorldCollisionQuery final {
public:
    [[nodiscard]] static bool Closest(const CollisionSegment& segment,
                                      RpWorld* world, CollisionSegmentHit& hit);
};

}  // namespace pal4
