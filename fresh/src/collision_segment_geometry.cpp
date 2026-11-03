#include "pal4/collision_segment_geometry.h"
#include "pal4/comparison_movement_collision_trace.h"

#include <limits>

namespace pal4 {

CollisionPoint3& CollisionSegmentGeometry::ProjectPointOnSegment(
    CollisionPoint3& output, const CollisionPoint3& point,
    const CollisionPoint3& start, const CollisionPoint3& end) {
    const CollisionPoint3 direction{
        end[0] - start[0], end[1] - start[1], end[2] - start[2]};
    // IDA5FE91C..5FE94E: each product and Y/X/Z sum uses PC24 before
    // subtracting the two dots. Widening them shifts vertical-edge projections.
    const auto dot = [&direction](const CollisionPoint3& value) {
        const float y = value[1] * direction[1];
        const float x = direction[0] * value[0];
        const float z = value[2] * direction[2];
        const float yx = y + x;
        return yx + z;
    };
    const float numerator = dot(point) - dot(start);
    if (!(numerator > 0.0f)) {
        output = start;
        ComparisonMovementCollisionScope::RecordSegment(point,start,end,output);
        return output;
    }
    const float length_squared = dot(direction);
    if (numerator >= length_squared) {
        output = end;
        ComparisonMovementCollisionScope::RecordSegment(point,start,end,output);
        return output;
    }
    const float fraction = numerator / length_squared;
    for (std::size_t axis = 0; axis < output.size(); ++axis) {
        const auto offset = static_cast<float>(direction[axis] * fraction);
        output[axis] = start[axis] + offset;
    }
    ComparisonMovementCollisionScope::RecordSegment(point,start,end,output);
    return output;
}

bool CollisionSegmentGeometry::IntersectWorld(
    const CollisionSegment* segment, const std::uint32_t world,
    CollisionSegmentHit& hit, const float unused_float,
    const std::uint32_t unused_word, ICollisionSegmentRuntime& runtime) {
    (void)unused_float;
    (void)unused_word;
    if (segment == nullptr || world == 0u) return false;
    const CollisionSegmentQuery query{*segment, 1u};
    hit.triangle = 0u;
    hit.fraction = std::numeric_limits<float>::max();
    runtime.TraverseIntersections(world, query, 0x5feae0u, hit);
    if (!(hit.fraction < std::numeric_limits<float>::max())) return false;
    for (std::size_t axis = 0u; axis < hit.point.size(); ++axis) {
        const auto delta = segment->end[axis] - segment->start[axis];
        hit.point[axis] = segment->start[axis] + hit.fraction * delta;
    }
    return true;
}

std::uint32_t CollisionSegmentGeometry::RecordClosestHit(
    const std::uint32_t triangle, const float fraction,
    CollisionSegmentHit& hit) {
    if (fraction < hit.fraction) {
        hit.triangle = triangle;
        hit.fraction = fraction;
    }
    return triangle;
}

}  // namespace pal4
