#include "pal4/collision_tree_storage.h"

#include "pal4/d3d_vector3_operations.h"
#include "pal4/d3d_vector2_operations.h"

#include <bit>
#include <cmath>
#include <stdexcept>

namespace pal4 {

CollisionTreeStorageState& CollisionTreeStorage::Initialize(
    CollisionTreeStorageState& state, const std::uint8_t initial_flag) {
    auto& words = state.words;
    words[0] = (words[0] & 0xffffff00u) | initial_flag;
    words[1] = 0;
    words[2] = 0;
    words[3] = 0;
    words[4] = 0;
    words[5] = 0;
    words[7] = (words[7] & 0xffffff00u) | initial_flag;
    words[8] = 0;
    words[9] = 0;
    words[10] = 0;
    words[15] = 100;
    return state;
}

std::uint32_t CollisionTreeStorage::ClearItems(
    CollisionTreeStorageState& state,
    ICollisionTreeStorageRuntime& runtime) {
    const auto old_end = state.words[2];
    for (auto cursor = state.words[1]; cursor != old_end; cursor += 4u) {
        const auto item = runtime.ReadWord(cursor);
        if (item != 0u) {
            runtime.DestroyItem(item);
            static_cast<void>(runtime.Free(item));
        }
    }
    state.words[2] = state.words[1];
    return old_end;
}

std::uint32_t CollisionTreeStorage::Release(
    CollisionTreeStorageState& state,
    ICollisionTreeStorageRuntime& runtime) {
    static_cast<void>(ClearItems(state, runtime));
    static_cast<void>(runtime.Free(state.words[8]));
    state.words[8] = 0;
    state.words[9] = 0;
    state.words[10] = 0;
    const auto result = runtime.Free(state.words[1]);
    state.words[1] = 0;
    state.words[2] = 0;
    state.words[3] = 0;
    return result;
}

std::uint32_t CollisionTreeStorage::AppendTriangle(
    CollisionTreeStorageState& state,
    const D3dTriangleVector3& first,
    const D3dTriangleVector3& second,
    const D3dTriangleVector3& third,
    ICollisionTreeAppendRuntime& runtime) {
    const auto face = runtime.Allocate(240u);
    runtime.ConstructFace(face, first, second, third);

    const auto old_begin = state.words[1];
    const auto old_end = state.words[2];
    const auto old_capacity_end = state.words[3];
    const auto available = static_cast<std::int32_t>(
        old_capacity_end - old_end) >> 2;
    if (static_cast<std::uint32_t>(available) >= 1u) {
        runtime.WriteWord(old_end, face);
        state.words[2] = old_end + 4u;
        return state.words[2];
    }

    const auto size = old_begin == 0u ? 0u :
        static_cast<std::uint32_t>(
            static_cast<std::int32_t>(old_end - old_begin) >> 2);
    const auto growth = old_begin == 0u || size <= 1u ? 1u : size;
    const auto new_capacity = size + growth;
    const auto allocation_count =
        static_cast<std::int32_t>(new_capacity) < 0 ? 0u : new_capacity;
    const auto new_begin = runtime.Allocate(allocation_count * 4u);
    auto destination = new_begin;
    for (auto source = old_begin; source != old_end; source += 4u) {
        const auto value = runtime.ReadWord(source);
        if (new_begin != 0u) runtime.WriteWord(destination, value);
        destination += 4u;
    }
    if (new_begin != 0u) runtime.WriteWord(destination, face);
    static_cast<void>(runtime.Free(old_begin));
    state.words[3] = new_begin + new_capacity * 4u;
    state.words[2] = new_begin + size * 4u + 4u;
    state.words[1] = new_begin;
    return size;
}

std::uint32_t CollisionTreeStorage::SetDebugRenderer(
    CollisionTreeStorageState& state, const std::uint32_t callback) {
    state.words[4] = callback;
    return callback;
}

std::uint32_t CollisionTreeStorage::SelectFace(
    const CollisionTreeStorageState& state,
    const D3dTriangleVector3& point,
    ICollisionTreeFaceQueryRuntime& runtime) {
    float best_height_difference = std::bit_cast<float>(0x7f7fc99eu);
    float best_distance = std::bit_cast<float>(0x7f7fc99eu);
    bool have_direct_face = false;
    std::uint32_t selected = 0u;

    for (auto cursor = state.words[1]; cursor != state.words[2];
         cursor += 4u) {
        const auto face = runtime.ReadFacePointer(cursor);
        if (runtime.IsOutsideEdges3D(face, point)) {
            auto projected = point;
            runtime.ProjectHeight(face, projected);
            const auto difference = std::fabs(
                static_cast<double>(projected.y) - point.y);
            if (!have_direct_face || difference < best_height_difference) {
                best_height_difference = static_cast<float>(difference);
                selected = face;
                have_direct_face = true;
            }
            continue;
        }
        if (have_direct_face) continue;

        const auto centroid = runtime.Centroid(face);
        D3dTriangleVector2 intersection{};
        if (runtime.TraceToPoint(face, {centroid.x, centroid.z},
                                 {point.x, point.z}, intersection) != 2u) {
            continue;
        }
        D3dTriangleVector3 projected{intersection.x, 0.0f,
                                     intersection.z};
        runtime.ProjectHeight(face, projected);
        D3dVector3Operations::SubtractInPlace(projected, point);
        const auto distance = D3dVector3Operations::Length(projected);
        if (distance < best_distance) {
            best_distance = static_cast<float>(distance);
            selected = face;
        }
    }
    return selected;
}

bool CollisionTreeStorage::TraceFacePath(
    CollisionTreeStorageState& state, const std::uint32_t actor,
    const std::uint32_t start_face, const D3dTriangleVector3& start,
    const std::uint32_t target_face, const D3dTriangleVector3& target,
    ICollisionTreePathRuntime& runtime) {
    D3dFaceTraversalState traversal{};
    for (std::size_t index = 0u; index < traversal.words.size(); ++index) {
        traversal.words[index] = state.words[6u + index];
    }
    const auto save_traversal = [&]() {
        for (std::size_t index = 0u;
             index < traversal.words.size(); ++index) {
            state.words[6u + index] = traversal.words[index];
        }
    };

    const auto generation = ++state.words[5u];
    static_cast<void>(D3dFaceTraversal::ResetSeed(
        traversal, generation, start));
    runtime.InitializeSeedFace(target_face, traversal);

    if (!D3dFaceTraversal::HasCandidates(traversal)) {
        save_traversal();
        return false;
    }

    bool found = false;
    do {
        D3dFaceCandidate candidate{};
        static_cast<void>(D3dFaceTraversal::PopFace(
            traversal, candidate, runtime));
        if (candidate.face == start_face) {
            found = true;
        } else {
            runtime.PropagateFace(candidate.face, traversal);
        }
    } while (!found && D3dFaceTraversal::HasCandidates(traversal));

    save_traversal();
    if (!found) return false;

    runtime.BeginActorPath(actor, state, start, start_face,
                           target, target_face);
    if (start_face != 0u) {
        auto current_face = start_face;
        while (current_face != target_face && current_face != 0u) {
            const auto marker = runtime.Marker(current_face);
            const auto midpoint = runtime.Midpoint(current_face, marker);
            const auto projected = runtime.Project(current_face, midpoint);
            runtime.AddActorPoint(actor, projected, current_face);
            current_face = runtime.Neighbor(current_face, marker);
        }
    }
    runtime.EndActorPath(actor);
    return true;
}

std::uint32_t CollisionTreeStorage::AdvanceAcrossFaces(
    const CollisionTreeStorageState& state,
    const D3dTriangleVector3& start, const std::uint32_t initial_face,
    D3dTriangleVector3& destination, std::uint32_t& final_face,
    ICollisionTreeMovementRuntime& runtime) {
    D3dLine2State line{};
    D3dLine2::SetEndpoints(line, {start.x, start.z},
                           {destination.x, destination.z});
    auto current_face = initial_face;
    std::uint32_t status = 0u;
    std::int32_t steps = 0;

    while (D3dVector2Operations::NotEqual(line.start, line.end)) {
        std::uint32_t neighbor = 0u;
        std::uint32_t edge = 0u;
        D3dTriangleVector2 intersection{};
        status = runtime.Trace(current_face, line, neighbor, edge,
                               intersection);
        if (status == 2u) {
            ++steps;
            if (steps >= std::bit_cast<std::int32_t>(state.words[15])) {
                status = 3u;
                break;
            }
            D3dLine2::SetStart(line, intersection);
            if (neighbor != 0u) {
                current_face = neighbor;
            } else {
                runtime.AdjustEdge(current_face, edge, line);
                D3dTriangleVector2 remainder{};
                D3dVector2Operations::Subtract(remainder, line.end,
                                               line.start);
                D3dVector2Operations::ScaleInPlace(remainder, 0.89999998f);
                D3dTriangleVector2 shortened{};
                D3dVector2Operations::Add(shortened, line.start,
                                          remainder);
                D3dLine2::SetEnd(line, shortened);
            }
            continue;
        }
        if (status == 0u) {
            auto clamped = line.start;
            runtime.ClampPoint(current_face, clamped);
            D3dLine2::SetStart(line, clamped);
            break;
        }
        if (status == 1u) break;
    }

    final_face = current_face;
    destination.x = line.end.x;
    destination.z = line.end.z;
    runtime.ProjectHeight(current_face, destination);
    return status;
}

std::uint32_t CollisionTreeStorage::LinkFaceNeighbors(
    const CollisionTreeStorageState& state,
    ICollisionTreeNeighborRuntime& runtime) {
    for (auto source_slot = state.words[1]; source_slot != state.words[2];
         source_slot += 4u) {
        const auto source_address = runtime.ReadFacePointer(source_slot);
        auto& source = runtime.Face(source_address);
        for (auto candidate_slot = state.words[1];
             candidate_slot != state.words[2]; candidate_slot += 4u) {
            if (candidate_slot == source_slot) continue;
            const auto candidate_address =
                runtime.ReadFacePointer(candidate_slot);
            auto& candidate = runtime.Face(candidate_address);
            for (std::uint32_t edge = 0u; edge < 3u; ++edge) {
                if (D3dTriangleFace::Neighbor(source, edge) != 0u) continue;
                const auto first_vertex = edge;
                const auto last_vertex = (edge + 1u) % 3u;
                const auto vertex = [&](const std::uint32_t index) {
                    const auto offset = 28u + index * 12u;
                    return D3dTriangleVector3{
                        D3dTriangleFace::FloatAt(source, offset),
                        D3dTriangleFace::FloatAt(source, offset + 4u),
                        D3dTriangleFace::FloatAt(source, offset + 8u)};
                };
                if (D3dTriangleFace::LinkNeighbor(
                        candidate, vertex(first_vertex),
                        vertex(last_vertex), source_address) != 0u) {
                    static_cast<void>(D3dTriangleFace::SetNeighbor(
                        source, edge, candidate_address));
                    break;
                }
            }
        }
    }
    return state.words[2];
}

std::int32_t CollisionTreeStorage::FaceCount(
    const CollisionTreeStorageState& state) {
    if (state.words[1] == 0u) return 0;
    return std::bit_cast<std::int32_t>(state.words[2] - state.words[1]) >> 2;
}

std::uint32_t CollisionTreeStorage::FaceAt(
    const CollisionTreeStorageState& state, const std::uint32_t index,
    ICollisionTreeNeighborRuntime& runtime) {
    if (state.words[1] == 0u ||
        static_cast<std::uint32_t>(FaceCount(state)) <= index) {
        ThrowInvalidFaceIndex();
    }
    return runtime.ReadFacePointer(state.words[1] + index * 4u);
}

void CollisionTreeStorage::ThrowInvalidFaceIndex() {
    throw std::out_of_range("invalid vector<T> subscript");
}

std::uint32_t CollisionTreeStorage::SnapPointToFace(
    const std::uint32_t tree, D3dTriangleVector3& point,
    ICollisionTreePointProjectionRuntime& runtime) {
    const auto face = runtime.SelectFace(tree, point);
    point = runtime.Project(tree, face, point);
    if (!runtime.IsOutsideEdges(face, point)) {
        runtime.ClampPoint(face, point);
    }
    runtime.ProjectHeight(face, point);
    return face;
}

}  // namespace pal4
