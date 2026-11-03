#pragma once

#include "pal4/collision_triangle_containment.h"

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CollisionSegment final {
    CollisionPoint3 start{};
    CollisionPoint3 end{};
};

static_assert(sizeof(CollisionSegment) == 24);

struct CollisionSegmentQuery final {
    CollisionSegment segment{};
    std::uint32_t type = 1u;
};

static_assert(sizeof(CollisionSegmentQuery) == 28);

struct CollisionSegmentHit final {
    std::uint32_t triangle = 0u;
    float fraction = 0.0f;
    CollisionPoint3 point{};
};

static_assert(offsetof(CollisionSegmentHit, fraction) == 4);
static_assert(offsetof(CollisionSegmentHit, point) == 8);

class ICollisionSegmentRuntime {
public:
    virtual ~ICollisionSegmentRuntime() = default;
    virtual void TraverseIntersections(
        std::uint32_t world, const CollisionSegmentQuery& query,
        std::uint32_t callback, CollisionSegmentHit& hit) = 0;
};

class CollisionSegmentRuntimeStub final : public ICollisionSegmentRuntime {
public:
    void TraverseIntersections(
        std::uint32_t, const CollisionSegmentQuery&,
        std::uint32_t, CollisionSegmentHit&) override {}
};

class CollisionSegmentGeometry final {
public:
    // IDA 0x5fe8f0.
    static CollisionPoint3& ProjectPointOnSegment(
        CollisionPoint3& output, const CollisionPoint3& point,
        const CollisionPoint3& start, const CollisionPoint3& end);
    // IDA 0x5fea10.
    [[nodiscard]] static bool IntersectWorld(
        const CollisionSegment* segment, std::uint32_t world,
        CollisionSegmentHit& hit, float unused_float,
        std::uint32_t unused_word, ICollisionSegmentRuntime& runtime);
    // IDA 0x5feae0, a tail block within 0x5fea10.
    [[nodiscard]] static std::uint32_t RecordClosestHit(
        std::uint32_t triangle, float fraction, CollisionSegmentHit& hit);
};

}  // namespace pal4
