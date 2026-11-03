#include "pal4/object_collision_processor.h"

namespace pal4 {
namespace {

BehaviorVector3 Difference(const BehaviorVector3& predicted,
                           const BehaviorVector3& current,
                           const BehaviorVector3& offset) {
    return {predicted[0] - current[0] - offset[0],
            predicted[1] - current[1] - offset[1],
            predicted[2] - current[2] - offset[2]};
}

BehaviorVector3& CompleteResponse(
    ObjectMovementState& state, BehaviorVector3& output,
    const BehaviorVector3& motion, ObjectCollisionSweep& hit,
    BehaviorVector3& predicted, IObjectCollisionRuntime& runtime) {
    runtime.Adjust(state, motion, hit.position, predicted, hit.time);
    const auto current = runtime.Position(state);
    const auto offset = runtime.Offset(state);
    output = Difference(predicted, current, offset);
    return output;
}

}  // namespace

float ObjectCollisionRuntimeStub::MovementScale(ObjectMovementState&) {
    return 0.0f;
}
BehaviorVector3 ObjectCollisionRuntimeStub::Motion(ObjectMovementState&, float) {
    return {};
}
BehaviorVector3 ObjectCollisionRuntimeStub::Position(ObjectMovementState&) {
    return {};
}
float ObjectCollisionRuntimeStub::Measure(
    std::uint32_t, const BehaviorVector3&) { return 0.0f; }
ObjectCollisionSweep ObjectCollisionRuntimeStub::Sweep(
    std::uint32_t, const BehaviorVector3&) { return {}; }
BehaviorVector3 ObjectCollisionRuntimeStub::Predict(
    std::uint32_t, const BehaviorVector3&, float) { return {}; }
void ObjectCollisionRuntimeStub::Adjust(
    ObjectMovementState&, const BehaviorVector3&,
    BehaviorVector3&, BehaviorVector3&, float) {}
BehaviorVector3 ObjectCollisionRuntimeStub::Offset(ObjectMovementState&) {
    return {};
}
BehaviorVector3 ObjectCollisionRuntimeStub::GlobalZeroVector() { return {}; }

BehaviorVector3& ObjectCollisionProcessor::ProcessClamped(
    ObjectMovementState& state, BehaviorVector3& output,
    std::int32_t option, float delta, std::uint32_t target,
    IObjectCollisionRuntime& runtime) {
    const float range = runtime.MovementScale(state) *
                        static_cast<float>(option) * delta;
    const auto motion = runtime.Motion(state, delta);
    const auto current = runtime.Position(state);
    const float current_measure = runtime.Measure(target, current);
    const float motion_measure = runtime.Measure(target, motion);
    const bool sign_gate = range > 0.0f
        ? current_measure < motion_measure
        : current_measure > motion_measure;

    auto hit = runtime.Sweep(target, motion);
    if (hit.time < 0.0f && sign_gate) {
        output = runtime.GlobalZeroVector();
        return output;
    }

    auto predicted = runtime.Predict(target, current, current_measure + range);
    return CompleteResponse(state, output, motion, hit, predicted, runtime);
}

BehaviorVector3& ObjectCollisionProcessor::ProcessUnclamped(
    ObjectMovementState& state, BehaviorVector3& output,
    float delta, std::uint32_t target,
    IObjectCollisionRuntime& runtime) {
    const auto motion = runtime.Motion(state, delta);
    auto hit = runtime.Sweep(target, motion);
    if (hit.time < 0.0f) {
        output = runtime.GlobalZeroVector();
        return output;
    }
    return CompleteResponse(state, output, motion, hit, hit.position, runtime);
}

}  // namespace pal4
