#pragma once

#include "pal4/collision_triangle_containment.h"

namespace pal4 {

class ICollisionPlaneRuntime {
public:
    virtual ~ICollisionPlaneRuntime() = default;
    virtual void Normalize(CollisionPoint3& normal) = 0;
};

class CollisionPlaneRuntimeStub final : public ICollisionPlaneRuntime {
public:
    void Normalize(CollisionPoint3&) override {}
};

class CollisionPlane final {
public:
    // IDA 0x5fecf0.
    [[nodiscard]] static double FromPoints(
        const CollisionPoint3& first, const CollisionPoint3& second,
        const CollisionPoint3& third, CollisionPoint3* normal,
        ICollisionPlaneRuntime& runtime);
};

}  // namespace pal4
