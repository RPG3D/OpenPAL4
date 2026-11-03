#include "pal4/object_movement_processor.h"
#include "pal4/object_collision_processor.h"
#include "pal4/object_complex_collision.h"
#include "pal4/object_collision_time.h"

#include <bit>

namespace pal4 {
namespace {

constexpr float kRandScale = 0.000030518509f;
constexpr float kRandThreshold = 0.1f;

bool PassesRandomGate(IObjectMovementRuntime& runtime) {
    const auto sample = runtime.Rand();
    return static_cast<double>(sample) * static_cast<double>(kRandScale) >
           static_cast<double>(kRandThreshold);
}

BehaviorVector3 Add(const BehaviorVector3& a, const BehaviorVector3& b) {
    return {a[0] + b[0], a[1] + b[1], a[2] + b[2]};
}

BehaviorVector3 Scale(const BehaviorVector3& value, float factor) {
    return {value[0] * factor, value[1] * factor, value[2] * factor};
}

bool Equal(const BehaviorVector3& a, const BehaviorVector3& b) {
    return a[0] == b[0] && a[1] == b[1] && a[2] == b[2];
}

float ReadFloat(const ObjectMovementState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

void WriteFloat(ObjectMovementState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
}

bool ReadByte(const ObjectMovementState& state, std::uint32_t offset) {
    return ((state.words[offset / 4u] >> (8u * (offset & 3u))) & 0xffu) != 0u;
}

}  // namespace

BehaviorVector3 ObjectMovementRuntimeStub::Position(ObjectMovementState&) { return {}; }
std::int32_t ObjectMovementRuntimeStub::Rand() { return 0; }
float ObjectMovementRuntimeStub::CandidateRange(ObjectMovementState&) { return 0.0f; }
void ObjectMovementRuntimeStub::CalculateCandidate(
    ObjectMovementState& state, std::uint32_t object,
    ObjectCollisionCandidate& candidate) {
    ObjectCollisionTimeRuntimeStub runtime;
    ObjectCollisionTime::Calculate(state, object, candidate, runtime);
}
void ObjectMovementRuntimeStub::NotifyCandidate(
    ObjectMovementState&, std::uint32_t, float) {}
BehaviorVector3 ObjectMovementRuntimeStub::CandidateResponse(
    ObjectMovementState&, const ObjectCollisionCandidate&, float) { return {}; }
BehaviorVector3 ObjectMovementRuntimeStub::ComparisonVector() { return {}; }
float ObjectMovementRuntimeStub::MovementRange(ObjectMovementState&) { return 0.0f; }
void ObjectMovementRuntimeStub::CopyGlobalCollection() {}
void ObjectMovementRuntimeStub::NotifyTarget(
    ObjectMovementState&, float, ObjectMovementTarget&) {}
BehaviorVector3 ObjectMovementRuntimeStub::ComplexCollision(
    ObjectMovementState& state, const BehaviorVector3&) {
    ObjectComplexCollisionRuntimeStub runtime;
    BehaviorVector3 output{};
    return ObjectComplexCollision::Process(state, 0u, output, 3.0f, {}, runtime);
}
float ObjectMovementRuntimeStub::RandomClamped(float value, float) { return value; }
BehaviorVector3 ObjectMovementRuntimeStub::DirectionDrift(
    ObjectMovementState&, float, float, const BehaviorVector3&) { return {}; }
std::uint32_t ObjectMovementRuntimeStub::ResolveCollisionProxy(std::uint32_t object) {
    return object;
}
BehaviorVector3 ObjectMovementRuntimeStub::ClampedCollision(
    ObjectMovementState& state, const BehaviorVector3&,
    std::uint32_t option, std::uint32_t proxy) {
    ObjectCollisionRuntimeStub runtime;
    BehaviorVector3 output{};
    return ObjectCollisionProcessor::ProcessClamped(
        state, output, static_cast<std::int32_t>(option), 2.5f, proxy, runtime);
}
BehaviorVector3 ObjectMovementRuntimeStub::UnclampedCollision(
    ObjectMovementState& state, const BehaviorVector3&, std::uint32_t proxy) {
    ObjectCollisionRuntimeStub runtime;
    BehaviorVector3 output{};
    return ObjectCollisionProcessor::ProcessUnclamped(
        state, output, 2.5f, proxy, runtime);
}

BehaviorVector3& ObjectMovementProcessor::Process(
    ObjectMovementState& state, BehaviorVector3& output, float delta,
    std::span<const std::uint32_t> candidates, IObjectMovementRuntime& runtime) {
    auto position = runtime.Position(state);
    BehaviorVector3 candidate_move{};

    if (PassesRandomGate(runtime)) {
        const float range = runtime.CandidateRange(state) * 4.0f;
        ObjectCollisionCandidate best{};
        best.InitializeBody();
        for (const auto object : candidates) {
            ObjectCollisionCandidate current{};
            current.InitializeBody();
            runtime.CalculateCandidate(state, object, current);
            if (!best.HasHit() ||
                (current.HasHit() && current.Time() < best.Time()))
                best.CopyFrom(current);
        }
        if (best.HasHit() && best.Time() < range) {
            runtime.NotifyCandidate(state, best.words[8], range);
            candidate_move = runtime.CandidateResponse(state, best, range);
        }
    }

    if (Equal(candidate_move, runtime.ComparisonVector())) {
        const float range = runtime.MovementRange(state) * 6.0f;
        runtime.CopyGlobalCollection();
        ObjectMovementTarget target{};
        runtime.NotifyTarget(state, range, target);

        BehaviorVector3 collision_move{};
        if (PassesRandomGate(runtime))
            collision_move = Scale(
                runtime.ComplexCollision(state, target.collision_input), 10.0f);

        if (Equal(collision_move, runtime.ComparisonVector())) {
            if (ReadByte(state, 0xc4u)) {
                const float radius = delta * 12.0f;
                const float first = runtime.RandomClamped(ReadFloat(state, 13u), radius);
                WriteFloat(state, 13u, first);
                const float second = runtime.RandomClamped(ReadFloat(state, 14u), radius);
                WriteFloat(state, 14u, second);
                position = Add(position, runtime.DirectionDrift(
                    state, first, second, target.collision_input));
            }

            const auto proxy = runtime.ResolveCollisionProxy(state.words[47]);
            const auto response = ReadByte(state, 0xc5u)
                ? runtime.ClampedCollision(state, target.collision_input,
                                           state.words[48], proxy)
                : runtime.UnclampedCollision(state, target.unclamped_input, proxy);
            position = Add(position, Scale(response, 0.5f));
        } else {
            position = Add(position, collision_move);
        }
    } else {
        position = Add(position, candidate_move);
    }

    output = {position[0], 0.0f, position[2]};
    return output;
}

}  // namespace pal4
