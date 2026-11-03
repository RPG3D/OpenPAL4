#include "pal4/object_random_movement_collision.h"

#include "pal4/object_collision_time.h"
#include "pal4/object_complex_collision.h"
#include "pal4/vector3_operations.h"

#include <bit>
#include <algorithm>

namespace pal4 {
namespace {

bool PassesRandomGate(IObjectRandomMovementCollisionRuntime& runtime) {
    constexpr float kRandScale = std::bit_cast<float>(0x38000100u);
    constexpr float kThreshold = std::bit_cast<float>(0x3dcccccdu);
    return static_cast<double>(runtime.Rand()) * kRandScale > kThreshold;
}

}  // namespace

BehaviorVector3 ObjectRandomMovementCollision::ObstacleCorrection(
    ObjectRandomMovementState& state, const ObjectCollisionCandidate& best,
    IObjectObstacleCorrectionRuntime& runtime) {
    const auto position = runtime.CollisionPosition(state);
    const auto candidate = runtime.CandidateVector(best.words[8] + 8u);
    BehaviorVector3 difference{};
    Vector3Operations::Subtract(position, difference, candidate);

    BehaviorVector3 projected{};
    const auto first_axis = runtime.CollisionAxis(state);
    Vector3Operations::SubtractProjection(difference, projected, first_axis);
    BehaviorVector3 normalized{};
    Vector3Operations::Normalize(projected, normalized);
    BehaviorVector3 scaled_projection{};
    Vector3Operations::ScaleInPlaceAndCopy(
        normalized, scaled_projection, runtime.CorrectionScale(state));

    const auto second_scale = runtime.CorrectionScale(state);
    const auto second_axis = runtime.CollisionAxis(state);
    BehaviorVector3 scaled_axis{};
    BehaviorVector3 damped_axis{};
    Vector3Operations::Scale(second_axis, scaled_axis, second_scale);
    Vector3Operations::Scale(scaled_axis, damped_axis, 0.75f);
    BehaviorVector3 correction{};
    Vector3Operations::AddInPlaceAndCopy(
        normalized, correction, damped_axis);
    return correction;
}

BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::Position(
    ObjectRandomMovementState&) { return {}; }
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::RandomVector() {
    return {};
}
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::BaseVector(
    ObjectRandomMovementState&) { return {}; }
std::int32_t ObjectRandomMovementCollisionRuntimeStub::Rand() { return 0; }
float ObjectRandomMovementCollisionRuntimeStub::Speed(
    ObjectRandomMovementState&) { return 0.0f; }
void ObjectRandomMovementCollisionRuntimeStub::CalculateCandidate(
    ObjectRandomMovementState& state, std::uint32_t candidate_address,
    ObjectCollisionCandidate& record) {
    ObjectMovementState prefix{};
    std::copy_n(state.words.begin(), prefix.words.size(), prefix.words.begin());
    ObjectCollisionTimeRuntimeStub runtime;
    ObjectCollisionTime::Calculate(prefix, candidate_address, record, runtime);
}
void ObjectRandomMovementCollisionRuntimeStub::NotifyCandidate(
    ObjectRandomMovementState&, std::uint32_t, float) {}
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::CollisionPosition(
    ObjectRandomMovementState& state) {
    return {std::bit_cast<float>(state.words[10]),
            std::bit_cast<float>(state.words[11]),
            std::bit_cast<float>(state.words[12])};
}
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::CandidateVector(
    std::uint32_t) { return {}; }
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::CollisionAxis(
    ObjectRandomMovementState& state) {
    return {std::bit_cast<float>(state.words[7]),
            std::bit_cast<float>(state.words[8]),
            std::bit_cast<float>(state.words[9])};
}
float ObjectRandomMovementCollisionRuntimeStub::CorrectionScale(
    ObjectRandomMovementState& state) {
    return std::bit_cast<float>(state.words[26]);
}
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::GlobalZeroVector() {
    return {};
}
float ObjectRandomMovementCollisionRuntimeStub::FallbackRange(
    ObjectRandomMovementState&) { return 0.0f; }
void ObjectRandomMovementCollisionRuntimeStub::CopyGlobalCollection() {}
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::NotifyMemberScan(
    ObjectRandomMovementState&, std::uint32_t, float) { return {}; }
BehaviorVector3 ObjectRandomMovementCollisionRuntimeStub::ComplexCollision(
    ObjectRandomMovementState& state, const BehaviorVector3&, float range) {
    ObjectMovementState prefix{};
    std::copy_n(state.words.begin(), prefix.words.size(), prefix.words.begin());
    ObjectComplexCollisionRuntimeStub runtime;
    BehaviorVector3 output{};
    ObjectComplexCollision::Process(prefix, 0u, output, range, {}, runtime);
    std::copy_n(prefix.words.begin() + 15u, 6u, state.words.begin() + 15u);
    return output;
}

BehaviorVector3& ObjectRandomMovementCollision::Process(
    ObjectRandomMovementState& state, BehaviorVector3& output,
    float delta_time, std::span<const std::uint32_t> candidates,
    IObjectRandomMovementCollisionRuntime& runtime) {
    const auto base = runtime.BaseVector(state);
    BehaviorVector3 correction{};

    if (PassesRandomGate(runtime)) {
        ObjectCollisionCandidate best{};
        ObjectCollisionCandidate trial{};
        best.InitializeBody();
        trial.InitializeBody();
        best.words[0] = 0u;
        trial.words[0] = 0u;
        const float range = runtime.Speed(state) * 4.0f;
        for (const auto candidate : candidates) {
            runtime.CalculateCandidate(state, candidate, trial);
            if (!best.HasHit() ||
                (trial.HasHit() && trial.Time() < best.Time())) {
                best.CopyFrom(trial);
            }
        }
        if (best.HasHit() && best.Time() < range) {
            runtime.NotifyCandidate(state, best.words[8], range);
            correction = ObstacleCorrection(state, best, runtime);
        }
    }

    const auto zero = runtime.GlobalZeroVector();
    BehaviorVector3 combined{};
    if (!Vector3Operations::Equals(correction, zero)) {
        Vector3Operations::Add(base, combined, correction);
    } else {
        const float range = runtime.FallbackRange(state) * 6.0f;
        runtime.CopyGlobalCollection();
        const auto position = runtime.NotifyMemberScan(
            state, state.words[42], range);
        BehaviorVector3 secondary{};
        if (PassesRandomGate(runtime)) {
            const auto complex = runtime.ComplexCollision(state, position, 3.0f);
            Vector3Operations::Scale(complex, secondary, 10.0f);
        }
        if (Vector3Operations::Equals(secondary, zero)) {
            static_cast<void>(ObjectRandomMovementStateUpdate::Update(
                state, delta_time, runtime));
            const float speed = runtime.Speed(state);
            const BehaviorVector3 heading{
                std::bit_cast<float>(state.words[50]),
                std::bit_cast<float>(state.words[51]),
                std::bit_cast<float>(state.words[52])};
            Vector3Operations::Scale(heading, secondary, speed);
        }
        Vector3Operations::Add(base, combined, secondary);
    }
    output = {combined[0], 0.0f, combined[2]};
    return output;
}

}  // namespace pal4
