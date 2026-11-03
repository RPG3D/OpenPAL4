#include "pal4/config_composite_collision.h"
#include "pal4/comparison_movement_collision_trace.h"

#include <array>
#include <bit>
#include <cmath>
#include <limits>

namespace pal4 {

bool ConfigCompositeCollision::CheckSecondList(
    const ConfigCompositeState& state,
    const ConfigCollisionPosition* start,
    ConfigCollisionPosition* position,
    CollisionPoint3& movement, const std::uint32_t,
    CollisionPoint3* normal, float* distance,
    IConfigCompositeCollisionRuntime& runtime) {
    if (start == nullptr || position == nullptr) return false;

    const auto head = state.words[5u];
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        const auto atomic = runtime.ReadWord(node + 8u);
        const auto hard_block = runtime.ReadWord(node + 20u);
        CollisionSegment segment{};
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            segment.start[axis] = (*position)[axis];
            segment.end[axis] = (*position)[axis];
        }

        bool hit = runtime.CheckLineSegment(
            *start, atomic, movement, normal, distance, 1u);
        if (hit) {
            for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                (*position)[axis] += movement[axis];
            }
        }
        if (runtime.CheckLineSegment(
                *position, atomic, movement, normal, distance, 1u)) {
            hit = true;
        }
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            segment.end[axis] += movement[axis];
        }

        if (hard_block != 0u && !hit) {
            ConfigCollisionScratch scratch{};
            if (runtime.ProcessWorldCollision(segment, atomic, scratch)) {
                movement.fill(0.0f);
                return true;
            }
        }
        if (hit) return true;
    }
    return false;
}

bool ConfigCompositeCollision::CheckThirdList(
    const ConfigCompositeState& state,
    const ConfigCollisionPosition* start,
    const ConfigCollisionPosition* position,
    CollisionPoint3& movement, std::uint32_t& hard_blocked,
    const std::uint32_t, const std::uint32_t,
    IConfigCompositeCollisionRuntime& runtime) {
    if (start == nullptr || position == nullptr) return false;

    hard_blocked = 0u;
    CollisionSegment position_segment{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        position_segment.start[axis] = (*position)[axis];
        position_segment.end[axis] = (*position)[axis] + movement[axis];
    }
    CollisionPoint3 first_normal{};
    CollisionPoint3 second_normal{};
    float first_distance = 0.0f;
    float second_distance = 0.0f;
    bool hit = false;
    const auto head = state.words[8u];
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        const auto atomic = runtime.ReadWord(node + 8u);
        const auto hard_block = runtime.ReadWord(node + 20u);
        if (hard_block != 0u) {
            ConfigCollisionScratch scratch{};
            if (runtime.ProcessWorldCollision(
                    position_segment, atomic, scratch)) {
                movement.fill(0.0f);
                hard_blocked = 1u;
                return true;
            }
        }
        if (runtime.CheckLineSegment(*start, atomic, movement,
                                     &first_normal, &first_distance, 1u)) {
            hit = true;
        }
        if (runtime.CheckLineSegment(*position, atomic, movement,
                                     &second_normal, &second_distance, 1u)) {
            hit = true;
        }
    }
    if (!hit || hard_blocked != 0u) return hit;

    CollisionPoint3 normalized{};
    runtime.NormalizeMovement(normalized, movement);
    CollisionSegment forward_segment{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        forward_segment.start[axis] = (*start)[axis];
        forward_segment.end[axis] = (*start)[axis] +
            (*start)[3u] * 1.5f * normalized[axis];
    }
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        ConfigCollisionScratch scratch{};
        if (runtime.ProcessWorldCollision(
                forward_segment, runtime.ReadWord(node + 8u), scratch)) {
            movement.fill(0.0f);
            break;
        }
    }
    return true;
}

bool ConfigCompositeCollision::CheckFourthList(
    ConfigCompositeState& state,
    const ConfigCollisionPosition* start,
    const ConfigCollisionPosition* position,
    const std::uint32_t excluded_type, CollisionPoint3& movement,
    std::uint32_t& hard_blocked, const std::uint32_t,
    const std::uint32_t, IConfigCompositeCollisionRuntime& runtime) {
    ComparisonMovementCollisionScope observation(start?start->data():nullptr,
        position?position->data():nullptr,movement.data(),&hard_blocked,
        reinterpret_cast<std::uintptr_t>(&state));
    if (start == nullptr || position == nullptr) return false;

    state.words[22u] = 0u;
    hard_blocked = 0u;
    CollisionSegment position_segment{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        position_segment.start[axis] = (*position)[axis];
        position_segment.end[axis] = (*position)[axis] + movement[axis];
    }
    CollisionPoint3 first_normal{};
    CollisionPoint3 second_normal{};
    float first_distance = 0.0f;
    float second_distance = 0.0f;
    bool hit = false;
    const auto head = state.words[11u];
    for (auto node = runtime.ReadWord(head); node != head;
         node = runtime.ReadWord(node)) {
        const auto atomic = runtime.ReadWord(node + 8u);
        if (runtime.ReadWord(atomic + 60u) == excluded_type) continue;
        if (runtime.HasPatchMesh(atomic) && !runtime.HasHierarchy(atomic)) {
            continue;
        }
        if (!runtime.HasPluginData(atomic)) continue;

        const auto mode = runtime.ReadWord(node + 12u);
        if (runtime.ReadWord(node + 20u) != 0u && mode != 0u) {
            ConfigCollisionScratch scratch{};
            if (runtime.ProcessWorldCollision(
                    position_segment, atomic, scratch)) {
                movement.fill(0.0f);
                hard_blocked = 1u;
                return true;
            }
        }

        bool node_hit = runtime.CheckLineSegment(
            *start, atomic, movement, &first_normal, &first_distance, mode);
        if (runtime.CheckLineSegment(
                *position, atomic, movement, &second_normal,
                &second_distance, mode)) {
            node_hit = true;
        }
        if (!node_hit && runtime.ReadWord(node + 24u) != 0u) {
            CollisionSegment proximity{};
            for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                proximity.start[axis] = (*position)[axis];
                proximity.end[axis] = (*position)[axis];
            }
            proximity.end[1u] -= (*position)[3u] * 2.5f;
            ConfigCollisionScratch scratch{};
            node_hit = runtime.ProcessWorldCollision(
                proximity, atomic, scratch);
        }

        const auto callback = runtime.ReadWord(node + 28u);
        if (node_hit) {
            hit = true;
            if (callback != 0u) {
                runtime.NotifyEnter(callback,
                                    runtime.ReadWord(node + 16u), state);
                runtime.WriteWord(node + 16u, 1u);
            }
        } else if (runtime.ReadWord(node + 16u) != 0u && callback != 0u) {
            runtime.NotifyLeave(callback, state);
            runtime.WriteWord(node + 16u, 0u);
        }
        if (state.words[22u] != 0u) {
            state.words[22u] = 0u;
            return hit;
        }
    }
    return hit;
}

const ConfigCollisionTriangle&
ConfigCompositeCollision::RecordClosestTrianglePoint(
    const CollisionPoint3& query,
    const ConfigCollisionTriangle& triangle,
    const std::uint32_t,
    ConfigCollisionClosestPoint& closest,
    IConfigCompositeCollisionRuntime& runtime) {
    std::array<CollisionPoint3, 3u> vertices{};
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        vertices[index] = runtime.VertexWorldPosition(
            triangle.vertex_references[index], closest.object_transform);
    }
    const auto transformed_normal = runtime.TransformNormal(
        triangle.normal, closest.object_transform);
    CollisionPoint3 normal{};
    runtime.NormalizeMovement(normal, transformed_normal);

    const auto plane_distance =
        (vertices[0u][0u] * normal[0u] +
         vertices[0u][1u] * normal[1u] +
         vertices[0u][2u] * normal[2u]) -
        (query[0u] * normal[0u] +
         query[1u] * normal[1u] +
         query[2u] * normal[2u]);
    CollisionPoint3 projected{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        projected[axis] = query[axis] + normal[axis] * plane_distance;
    }
    if (CollisionTriangleContainment::ContainsProjectedPoint(
            projected, vertices, normal)) {
        const auto candidate_distance = std::fabs(plane_distance);
        if (candidate_distance < closest.distance) {
            closest.distance = candidate_distance;
            closest.point = projected;
        }
    } else {
        for (std::uint32_t index = 0u; index < 3u; ++index) {
            CollisionPoint3 edge_point{};
            (void)CollisionSegmentGeometry::ProjectPointOnSegment(
                edge_point, projected, vertices[index],
                vertices[(index + 1u) % 3u]);
            CollisionPoint3 delta{};
            for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                delta[axis] = query[axis] - edge_point[axis];
            }
            const auto candidate_distance = runtime.VectorMagnitudeApprox(delta);
            if (candidate_distance < closest.distance) {
                closest.distance = candidate_distance;
                closest.point = edge_point;
            }
        }
    }
    return triangle;
}

bool ConfigCompositeCollision::CheckLineSegmentIntersection(
    const ConfigCollisionPosition* start, const std::uint32_t atomic,
    CollisionPoint3& movement, CollisionPoint3* normal,
    float* distance, const std::uint32_t mode,
    IConfigCompositeCollisionRuntime& runtime) {
    if (start == nullptr || atomic == 0u) return false;

    ConfigCollisionSphereQuery query{};
    for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
        query.center[axis] = (*start)[axis] + movement[axis];
    }
    query.radius = (*start)[3u];
    ConfigCollisionClosestPoint closest{};
    closest.object_transform = runtime.ObjectTransform(
        runtime.ReadWord(atomic + 4u));
    closest.distance = std::numeric_limits<float>::max();
    runtime.ForAllAtomicIntersections(
        atomic, query, 0x5ffcb0u, closest);
    if (!(closest.distance < std::numeric_limits<float>::max())) {
        return false;
    }
    if (mode != 0u) {
        CollisionPoint3 delta{};
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            delta[axis] = query.center[axis] - closest.point[axis];
        }
        CollisionPoint3 normalized{};
        runtime.NormalizeMovement(normalized, delta);
        if (normal != nullptr) {
            for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
                normalized[axis] += (*normal)[axis];
            }
        }
        const auto remaining = query.radius - closest.distance;
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            movement[axis] += normalized[axis] * remaining;
        }
        if (normal != nullptr) *normal = normalized;
        if (distance != nullptr) *distance = remaining;
    }
    return true;
}

std::uint32_t ConfigCompositeCollision::SharedIntersectionReturn(
    const std::uint32_t result) noexcept {
    return result;
}

std::uint32_t ConfigCompositeCollision::CheckFloorClumpIntersection(
    const ConfigCompositeState& state, const CollisionPoint3& point,
    const float upper_offset, const float lower_offset,
    CollisionPoint3* adjusted_point, std::uint32_t* second_result,
    std::uint32_t* first_result,
    IConfigFloorClumpCollisionRuntime& runtime) {
    const auto clump = state.words[13u];
    if (clump == 0u) return 0u;

    CollisionSegment segment{point, point};
    segment.start[1u] += upper_offset;
    segment.end[1u] -= lower_offset;
    ConfigCollisionScratch scratch{};
    const auto result = runtime.ProcessClumpCollision(segment, clump, scratch);
    if (result != 0u && adjusted_point != nullptr) {
        if (first_result != nullptr) *first_result = scratch[0u];
        if (second_result != nullptr) *second_result = scratch[1u];
        *adjusted_point = point;
        (*adjusted_point)[1u] = std::bit_cast<float>(scratch[4u]);
    }
    return result;
}

}  // namespace pal4
