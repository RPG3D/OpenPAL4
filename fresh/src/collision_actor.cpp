#include "pal4/collision_actor.h"

#include <bit>
#include <cmath>
#include <stdexcept>

#include "pal4/d3d_vector3_operations.h"

namespace pal4 {

namespace {

D3dTriangleVector3 ReadVector(const CollisionActorState& state,
                              const std::uint32_t first_word) {
    return {std::bit_cast<float>(state.words[first_word]),
            std::bit_cast<float>(state.words[first_word + 1u]),
            std::bit_cast<float>(state.words[first_word + 2u])};
}

void WriteVector(CollisionActorState& state, const std::uint32_t first_word,
                 const D3dTriangleVector3& value) {
    state.words[first_word] = std::bit_cast<std::uint32_t>(value.x);
    state.words[first_word + 1u] = std::bit_cast<std::uint32_t>(value.y);
    state.words[first_word + 2u] = std::bit_cast<std::uint32_t>(value.z);
}

D3dTriangleVector3 ReadNodePoint(const std::uint32_t node,
                                 ID3dFacePathRuntime& runtime) {
    return {std::bit_cast<float>(runtime.ReadWord(node + 8u)),
            std::bit_cast<float>(runtime.ReadWord(node + 12u)),
            std::bit_cast<float>(runtime.ReadWord(node + 16u))};
}

void ClearVelocity(CollisionActorState& state) {
    state.words[6u] = 0u;
    state.words[7u] = 0u;
    state.words[8u] = 0u;
}

}  // namespace

CollisionActorState& CollisionActor::Construct(
    CollisionActorState& state, const std::uint8_t path_flag,
    ID3dFacePathRuntime& runtime) {
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    for (std::uint32_t index = 3u; index <= 8u; ++index) {
        state.words[index] = 0u;
    }
    state.words[9u] = 0x42c96148u;
    state.words[10u] &= 0xffffff00u;
    state.words[20u] = (state.words[20u] & 0xffffff00u) | path_flag;
    const auto sentinel = runtime.Allocate(24u);
    runtime.WriteWord(sentinel, sentinel);
    runtime.WriteWord(sentinel + 4u, sentinel);
    state.words[21u] = sentinel;
    state.words[22u] = 0u;
    for (std::uint32_t index = 24u; index <= 27u; ++index) {
        state.words[index] = 0u;
    }
    state.words[29u] = 0x40a00000u;
    state.words[36u] = 1u;
    state.words[0u] = kVtableAddress;
    return state;
}

std::uint32_t CollisionActor::Destroy(
    CollisionActorState& state, ID3dFacePathRuntime& runtime) {
    state.words[0u] = kVtableAddress;
    const auto sentinel = state.words[21u];
    for (auto node = runtime.ReadWord(sentinel); node != sentinel;) {
        const auto next = runtime.ReadWord(node);
        const auto previous = runtime.ReadWord(node + 4u);
        runtime.WriteWord(previous, next);
        runtime.WriteWord(next + 4u, previous);
        runtime.Free(node);
        --state.words[22u];
        node = next;
    }
    runtime.Free(sentinel);
    state.words[21u] = 0u;
    state.words[22u] = 0u;
    return 0u;
}

std::uint32_t CollisionActor::Place(
    CollisionActorState& state, const std::uint32_t actor_address,
    const std::uint32_t tree, const std::uint32_t parameter_116,
    const D3dTriangleVector3& point, const std::uint32_t face,
    const std::uint32_t parameter_112,
    ICollisionActorPlacementRuntime& runtime) {
    state.words[1u] = tree;
    state.words[3u] = std::bit_cast<std::uint32_t>(point.x);
    state.words[4u] = std::bit_cast<std::uint32_t>(point.y);
    state.words[5u] = std::bit_cast<std::uint32_t>(point.z);
    state.words[29u] = parameter_116;
    state.words[28u] = parameter_112;
    state.words[2u] = face;
    state.words[36u] = 1u;
    state.words[6u] = 0u;
    state.words[7u] = 0u;
    state.words[8u] = 0u;
    if (tree == 0u) return 0u;

    if (face == 0u) state.words[2u] = runtime.SelectFace(tree, point);
    const auto projected = runtime.Project(tree, state.words[2u], point);
    state.words[3u] = std::bit_cast<std::uint32_t>(projected.x);
    state.words[4u] = std::bit_cast<std::uint32_t>(projected.y);
    state.words[5u] = std::bit_cast<std::uint32_t>(projected.z);
    return actor_address + 12u;
}

std::uint32_t CollisionActor::ClearBinding(CollisionActorState& state) {
    state.words[36u] = 1u;
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[10u] &= 0xffffff00u;
    for (std::uint32_t index = 6u; index <= 8u; ++index) {
        state.words[index] = 0u;
    }
    for (std::uint32_t index = 24u; index <= 27u; ++index) {
        state.words[index] = 0u;
    }
    return 0u;
}

std::uint32_t CollisionActor::SetPathCallback(
    CollisionActorState& state, const std::uint32_t callback) {
    return state.words[24u] = callback;
}

std::uint32_t CollisionActor::SetMovementCallback(
    CollisionActorState& state, const std::uint32_t callback) {
    return state.words[25u] = callback;
}

std::uint32_t CollisionActor::SetOutputCallback(
    CollisionActorState& state, const std::uint32_t callback) {
    return state.words[26u] = callback;
}

std::uint32_t CollisionActor::SetCallback108(
    CollisionActorState& state, const std::uint32_t callback) {
    return state.words[27u] = callback;
}

std::uint8_t CollisionActor::DispatchCallbacks(
    const CollisionActorState& state, const std::uint32_t actor_address,
    const std::uint32_t tick,
    ICollisionActorCallbackRuntime& runtime) {
    if (state.words[26u] != 0u) {
        runtime.Output(state.words[26u], actor_address + 132u,
                       actor_address + 120u, tick);
    }
    if ((state.words[10u] & 0xffu) == 0u) return 0u;
    if (state.words[24u] != 0u) {
        runtime.Path(state.words[24u], actor_address + 44u, tick);
    }
    if (state.words[25u] == 0u) return 0u;
    return static_cast<std::uint8_t>(runtime.Move(
        state.words[25u], actor_address + 12u,
        state.words[23u] + 8u, tick));
}

std::uint32_t CollisionActor::UpdateMovement(
    CollisionActorState& state, const std::uint32_t actor_address,
    const std::uint32_t event, const float elapsed,
    ICollisionActorMovementRuntime& runtime) {
    const auto step = elapsed * std::bit_cast<float>(state.words[9u]);
    if (state.words[1u] == 0u) return 0u;

    auto position = ReadVector(state, 3u);
    auto velocity = ReadVector(state, 6u);
    if ((state.words[10u] & 0xffu) != 0u) {
        if (state.words[23u] == state.words[21u]) {
            state.words[10u] &= 0xffffff00u;
            velocity = {};
        } else {
            velocity = ReadNodePoint(state.words[23u], runtime);
            D3dVector3Operations::SubtractInPlace(velocity, position);
        }
        state.words[36u] = 0u;
    } else {
        D3dVector3Operations::ScaleInPlace(velocity, 0.75f);
        state.words[36u] = 1u;
    }

    if (D3dVector3Operations::Length(velocity) > step) {
        D3dVector3Operations::NormalizeInPlace(velocity);
        D3dVector3Operations::ScaleInPlace(velocity, step);
    }
    const auto zero_small = [](float& value) {
        if (std::fabs(static_cast<double>(value)) <
            0.004999999888241291) value = 0.0f;
    };
    zero_small(velocity.x);
    zero_small(velocity.y);
    zero_small(velocity.z);
    WriteVector(state, 6u, velocity);

    if (velocity.x == 0.0f && velocity.z == 0.0f) {
        if ((state.words[10u] & 0xffu) != 0u) {
            position = ReadNodePoint(state.words[23u], runtime);
            WriteVector(state, 3u, position);
            velocity = {};
            ClearVelocity(state);
            D3dFacePathState path{};
            for (std::uint32_t index = 0u; index < 12u; ++index) {
                path.words[index] = state.words[11u + index];
            }
            state.words[23u] = D3dFacePath::SkipReachable(
                path, state.words[23u], runtime, runtime);
            if (state.words[23u] == state.words[21u]) {
                state.words[10u] &= 0xffffff00u;
                ClearVelocity(state);
            }
        }
    } else {
        static_cast<void>(RedirectMovement(state, step, event, runtime));
        position = ReadVector(state, 3u);
        velocity = ReadVector(state, 6u);
        D3dTriangleVector3 destination{};
        D3dVector3Operations::Add(destination, position, velocity);
        std::uint32_t final_face = state.words[2u];
        const auto status = runtime.AdvanceAcrossFaces(
            state.words[1u], position, state.words[2u],
            destination, final_face);
        if (status == 0u || status == 3u) {
            state.words[36u] = status == 0u ? 3u : 2u;
            WriteVector(state, 37u, destination);
            state.words[10u] &= 0xffffff00u;
            ClearVelocity(state);
        } else {
            WriteVector(state, 3u, destination);
            state.words[2u] = final_face;
        }
    }

    position = ReadVector(state, 3u);
    const auto radius = std::bit_cast<float>(state.words[29u]);
    WriteVector(state, 30u, {position.x + radius,
                             position.y + radius,
                             position.z + radius});
    WriteVector(state, 33u, {position.x - radius,
                             position.y - radius,
                             position.z - radius});
    return actor_address + 132u;
}

std::uint32_t CollisionActor::StopMovement(CollisionActorState& state) {
    state.words[10u] &= 0xffffff00u;
    ClearVelocity(state);
    return 0u;
}

bool CollisionActor::RequestRandomPath(
    CollisionActorState& state, const std::uint32_t actor_address,
    const D3dTriangleVector3& start, const std::uint32_t extra,
    ICollisionActorPathRuntime& runtime) {
    const auto tree = state.words[1u];
    if (tree == 0u) return false;
    const auto count = runtime.FaceCount(tree);
    if (count == 0) throw std::domain_error("collision tree has no faces");
    const auto random = runtime.RandomValue();
    const auto index = static_cast<std::uint32_t>(random % count);
    const auto target_face = runtime.FaceAt(tree, index);
    const auto target = runtime.Centroid(target_face);
    return RequestPath(state, actor_address, start, target,
                       target_face, extra, runtime);
}

bool CollisionActor::RequestPath(
    CollisionActorState& state, const std::uint32_t actor_address,
    const D3dTriangleVector3& start, const D3dTriangleVector3& target,
    const std::uint32_t target_face, const std::uint32_t extra,
    ICollisionActorPathRuntime& runtime) {
    const auto tree = state.words[1u];
    if (tree == 0u) return false;
    ClearVelocity(state);
    WriteVector(state, 3u, start);
    const auto start_face = runtime.SelectFace(tree, start);
    state.words[2u] = start_face;
    if (start_face != 0u) {
        const auto projected = runtime.Project(tree, start_face, start);
        WriteVector(state, 3u, projected);
        const auto found = runtime.TracePath(
            tree, actor_address + 44u, start_face, projected,
            target, target_face, extra);
        state.words[10u] = (state.words[10u] & 0xffffff00u) |
            static_cast<std::uint32_t>(found);
        if (found) state.words[23u] = runtime.ReadWord(state.words[21u]);
    }
    return (state.words[10u] & 0xffu) != 0u;
}

bool CollisionActor::RedirectMovement(
    CollisionActorState& state, const float step,
    const std::uint32_t event,
    ICollisionActorMovementRuntime& runtime) {
    const auto callback = state.words[27u];
    if (callback == 0u) return false;
    const auto sentinel = state.words[21u];
    auto candidate = runtime.ReadWord(state.words[23u]);
    if (candidate == sentinel) return false;

    const auto position = ReadVector(state, 3u);
    const auto height = std::bit_cast<float>(state.words[28u]);
    auto start = position;
    start.y += height;
    runtime.ClampFacePoint(state.words[2u], start);

    bool found = false;
    while (candidate != sentinel) {
        const auto point = ReadNodePoint(candidate, runtime);
        auto end = point;
        end.y += height;
        if (!runtime.TestClearance(callback, start, end, event)) break;
        static_cast<void>(runtime.ReadWord(candidate + 20u));
        if (D3dFaceTraceReachability::CanReach(
                state.words[2u], position, point, runtime)) {
            state.words[23u] = candidate;
            found = true;
        }
        candidate = runtime.ReadWord(candidate);
    }
    if (!found) return false;

    auto velocity = ReadNodePoint(state.words[23u], runtime);
    D3dVector3Operations::SubtractInPlace(velocity, position);
    if (D3dVector3Operations::Length(velocity) > step) {
        D3dVector3Operations::NormalizeInPlace(velocity);
        D3dVector3Operations::ScaleInPlace(velocity, step);
    }
    const auto zero_small = [](float& value) {
        if (std::fabs(static_cast<double>(value)) <
            0.004999999888241291) value = 0.0f;
    };
    zero_small(velocity.x);
    zero_small(velocity.y);
    zero_small(velocity.z);
    WriteVector(state, 6u, velocity);
    return true;
}

std::uint32_t CollisionActor::AddVelocity(
    CollisionActorState& state, const std::uint32_t actor_address,
    const D3dTriangleVector3& added) {
    auto velocity = ReadVector(state, 6u);
    D3dVector3Operations::AddInPlace(velocity, added);
    WriteVector(state, 6u, velocity);
    return actor_address + 24u;
}

std::uint32_t CollisionActor::SetSpeedBits(
    CollisionActorState& state, const std::uint32_t speed_bits) {
    return state.words[9u] = speed_bits;
}

std::uint8_t CollisionActor::PathEnabled(
    const CollisionActorState& state) {
    return static_cast<std::uint8_t>(state.words[10u]);
}

std::uint32_t CollisionActor::PositionAddress(
    const std::uint32_t actor_address) {
    return actor_address + 12u;
}

}  // namespace pal4
