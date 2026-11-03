#include "pal4/pathway_grid.h"
#include "movement_vector_math.h"

#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace pal4 {
namespace {

float Float(const std::uint32_t bits) {
    return std::bit_cast<float>(bits);
}

std::uint32_t Bits(const float value) {
    return std::bit_cast<std::uint32_t>(value);
}

PathwayPoint ReadPoint(IPathwayGridRuntime& runtime,
                       const std::uint32_t base, const std::uint32_t index,
                       const std::uint32_t stride) {
    PathwayPoint point;
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        point.coordinates[axis] = Float(runtime.ReadWord(
            base + stride * index + 4u * axis));
    return point;
}

float Distance(const PathwayPoint& first, const PathwayPoint& second) {
    std::array<float,3> difference{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        difference[axis]=first.coordinates[axis]-second.coordinates[axis];
    }
    // 65E050 builds a stored difference then calls40BF20 and FSQRT.
    return movement_detail::Length(difference);
}

}  // namespace

float PathwayGrid::DistanceToSegment(
    PathwayGridState& state, const PathwayPoint& point,
    const PathwayPoint& first, const PathwayPoint& last) {
    std::array<float,3> difference{},direction{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        difference[axis]=point.coordinates[axis]-first.coordinates[axis];
        state.difference_bits[axis]=Bits(difference[axis]);
        direction[axis]=Float(state.direction_bits[axis]);
    }
    const float projection=movement_detail::Dot(direction,difference);
    state.projection_bits=Bits(projection);
    PathwayPoint nearest;
    if (projection < 0.f) {
        nearest = first;
        state.projection_bits = 0u;
    } else if (projection > Float(state.segment_length_bits)) {
        nearest = last;
        state.projection_bits = state.segment_length_bits;
    } else {
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            const volatile float offset=projection*direction[axis];
            nearest.coordinates[axis]=first.coordinates[axis]+offset;
        }
    }
    for (std::uint32_t axis = 0u; axis < 3u; ++axis)
        state.nearest_bits[axis] = Bits(nearest.coordinates[axis]);
    return Distance(point, nearest);
}

float PathwayGrid::FindClosestPosition(
    PathwayGridState& state, const PathwayPoint& point,
    IPathwayGridRuntime& runtime) {
    float best_distance = std::numeric_limits<float>::max();
    float best_position = 0.0f;
    float accumulated = 0.0f;
    for (std::uint32_t index = 1u; index < state.point_count; ++index) {
        state.segment_length_bits = runtime.ReadWord(state.lengths + 4u * index);
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            state.direction_bits[axis] = runtime.ReadWord(
                state.directions + 12u * index + 4u * axis);
        const auto first = ReadPoint(runtime, state.positions, index - 1u, 12u);
        const auto last = ReadPoint(runtime, state.positions, index, 12u);
        const float distance = DistanceToSegment(state, point, first, last);
        if (distance < best_distance) {
            best_distance = distance;
            best_position = accumulated + Float(state.projection_bits);
        }
        accumulated += Float(state.segment_length_bits);
    }
    return best_position;
}

PathwayPoint PathwayGrid::ProjectToTube(
    PathwayGridState& state, const PathwayPoint& point,
    PathwayPoint& direction, float& outside, IPathwayGridRuntime& runtime) {
    float best_distance = std::numeric_limits<float>::max();
    PathwayPoint nearest{};
    for (std::uint32_t index = 1u; index < state.point_count; ++index) {
        state.segment_length_bits = runtime.ReadWord(state.lengths + 4u * index);
        for (std::uint32_t axis = 0u; axis < 3u; ++axis)
            state.direction_bits[axis] = runtime.ReadWord(
                state.directions + 12u * index + 4u * axis);
        const auto first = ReadPoint(runtime, state.positions, index - 1u, 12u);
        const auto last = ReadPoint(runtime, state.positions, index, 12u);
        const float distance = DistanceToSegment(state, point, first, last);
        if (distance < best_distance) {
            best_distance = distance;
            for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                nearest.coordinates[axis] = Float(state.nearest_bits[axis]);
                direction.coordinates[axis] = Float(state.direction_bits[axis]);
            }
        }
    }
    outside = Distance(nearest, point) - Float(state.kind);
    return nearest;
}

PathwayPoint PathwayGrid::CoordinatesAt(
    PathwayGridState& state, float position,
    IPathwayGridRuntime& runtime) {
    if (state.point_count == 0u || state.positions == 0u)
        return {};
    if (position < 0.0f)
        return ReadPoint(runtime, state.positions, 0u, 12u);
    if (!(position < Float(state.total_length_bits)))
        return ReadPoint(runtime, state.positions,
                         state.point_count - 1u, 12u);
    for (std::uint32_t index = 1u; index < state.point_count; ++index) {
        const float length = Float(runtime.ReadWord(state.lengths + 4u * index));
        state.segment_length_bits = Bits(length);
        if (position <= length) {
            const auto first = ReadPoint(runtime, state.positions, index - 1u, 12u);
            const auto last = ReadPoint(runtime, state.positions, index, 12u);
            PathwayPoint result;
            const float fraction = length == 0.0f ? 0.0f : position / length;
            for (std::uint32_t axis = 0u; axis < 3u; ++axis)
                result.coordinates[axis] = first.coordinates[axis] +
                    (last.coordinates[axis] - first.coordinates[axis]) * fraction;
            return result;
        }
        position -= length;
    }
    return {};
}

}  // namespace pal4
