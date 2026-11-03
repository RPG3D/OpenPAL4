#include "pal4/gob_obstacle.h"
#include "pal4/gob_obstacle_physics.h"

#include <cmath>
#include <cstdint>

namespace pal4 {
namespace {

GobObstacleVector Scale(const GobObstacleVector vector, const float factor) {
    return {vector.x * factor, vector.y * factor, vector.z * factor};
}

void ApplyForce(IGobObstaclePhysicsRuntime& runtime,
                const GobObstacleVector force, const float elapsed) {
    runtime.ApplyActorWorldForce(force);
    runtime.TickFloorSound(elapsed);
}

}  // namespace

std::uint32_t GobObstacle::UpdateMachineAndApplyActorForce(
    const std::uint32_t object, const float elapsed,
    IGobObstaclePhysicsRuntime& runtime) {
    runtime.UpdateMachine(object, elapsed);
    const auto control = runtime.ActorControl();
    if (runtime.ReadWord(control) == 0u) return control;
    const auto actor = runtime.ReadWord(runtime.ActorControl() + 4u);
    if (actor == 0u) return control;

    GobObstacleVector displacement{
        runtime.ReadFloat(object + 0xacu) - runtime.ReadFloat(actor + 0xacu),
        0.0f,
        runtime.ReadFloat(object + 0xb4u) - runtime.ReadFloat(actor + 0xb4u)};
    const auto distance = runtime.HorizontalMagnitude(displacement);
    const auto outside = distance > runtime.ReadFloat(object + 0x19cu) ||
        runtime.ActorHeight(actor) <= std::fabs(
            runtime.ReadFloat(actor + 0xb0u) -
            runtime.ReadFloat(object + 0xb0u));
    if (outside) {
        if (runtime.ReadWord(object + 0x2f0u) != 0u) {
            const auto remaining = runtime.ReadFloat(object + 0x2ecu) - 0.75f;
            runtime.WriteFloat(object + 0x2ecu, remaining);
            if (remaining > 0.0f) {
                ApplyForce(runtime, Scale({
                    runtime.ReadFloat(object + 0x2e0u),
                    runtime.ReadFloat(object + 0x2e4u),
                    runtime.ReadFloat(object + 0x2e8u)}, remaining * elapsed),
                    elapsed);
                return UpdateLaunchEffectOrDialog(object, elapsed, runtime);
            }
            runtime.WriteWord(object + 0x2f0u, 0u);
        }
        return UpdateLaunchEffectOrDialog(object, elapsed, runtime);
    }

    const auto range = (runtime.ReadFloat(actor + 0xecu) +
                        runtime.ReadFloat(object + 0xecu)) * 0.80000001f;
    // IDA5A4F14..1D compares range to distance and skips equality.668490
    // receives only the vector; range is a gate, not a normalization scale.
    if (range < distance) {
        runtime.ClampVectorToRange(range, displacement);
        const auto attraction = runtime.ReadFloat(object + 0x2d8u);
        if (attraction > 0.0f)
            ApplyForce(runtime, Scale(displacement, attraction * elapsed),
                       elapsed);
    }
    const auto thrust = runtime.ReadFloat(object + 0x2dcu);
    if (thrust <= 0.0f)
        return UpdateLaunchEffectOrDialog(object, elapsed, runtime);
    if (runtime.ReadWord(object + 0x2f0u) == 0u) {
        ApplyEffectTypeToActor(object, runtime);
        GobObstacleVector direction;
        if (runtime.ReadWord(object + 0x2f4u) != 0u) {
            direction = Scale(runtime.ObjectForward(object), distance);
        } else {
            direction = Scale(displacement, -1.0f);
        }
        runtime.WriteFloat(object + 0x2e0u, direction.x);
        runtime.WriteFloat(object + 0x2e4u, direction.y);
        runtime.WriteFloat(object + 0x2e8u, direction.z);
        runtime.WriteWord(object + 0x2f0u, 1u);
        runtime.WriteFloat(object + 0x2ecu, thrust);
    }
    ApplyForce(runtime, Scale({
        runtime.ReadFloat(object + 0x2e0u),
        runtime.ReadFloat(object + 0x2e4u),
        runtime.ReadFloat(object + 0x2e8u)},
        runtime.ReadFloat(object + 0x2ecu) * elapsed), elapsed);
    return UpdateLaunchEffectOrDialog(object, elapsed, runtime);
}

void GobObstacle::ApplyEffectTypeToActor(
    const std::uint32_t object, IGobObstaclePhysicsRuntime& runtime) {
    switch (runtime.ReadWord(object + 0x2f8u)) {
    case 1u:
        runtime.ApplyPlayerStatus(runtime.ReadFloat(object + 0x304u));
        break;
    case 2u:
        runtime.StartPlayerStatus(runtime.ReadWord(object + 0x2fcu),
                                  runtime.ReadWord(object + 0x300u),
                                  runtime.ReadFloat(object + 0x304u));
        break;
    case 3u:
        runtime.BeginMovementStatus(runtime.ReadWord(object + 0x2fcu));
        break;
    default:
        break;
    }
}

std::uint32_t GobObstacle::UpdateLaunchEffectOrDialog(
    const std::uint32_t object, const float elapsed,
    IGobObstaclePhysicsRuntime& runtime) {
    const auto name_length = static_cast<std::int32_t>(
        runtime.ReadWord(object + 0x30cu));
    if (name_length <= 0) return static_cast<std::uint32_t>(name_length);
    const auto handle = object + kEffectHandleOffset;
    if (runtime.DialogNeedsUpdate(handle)) return 1u;

    const auto timer = runtime.ReadFloat(object + 0x31cu);
    const auto effect_length = static_cast<std::int32_t>(
        runtime.ReadWord(object + 0x324u));
    if (effect_length > 0 && timer >= 0.0f) {
        const auto remaining = timer - elapsed;
        runtime.WriteFloat(object + 0x31cu, remaining);
        if (remaining > 0.0f) return 0u;
        runtime.WriteFloat(object + 0x31cu, 0.0f);
        if (runtime.DialogNeedsUpdate(handle))
            runtime.UpdateDialogContent(handle);
        const auto effect = runtime.CreateEffect(runtime.ReadCString(
            runtime.ReadWord(object + 0x320u)));
        runtime.WriteWord(handle, effect.first);
        runtime.WriteWord(handle + 4u, effect.second);
        runtime.ResetTemporaryEffect();
        const auto clump = runtime.ReadWord(
            runtime.ReadWord(object + 0x14u) + 4u);
        return runtime.UpdateEffectProperties(handle, clump);
    }
    if (effect_length <= 0 && timer < 0.0f) return 0u;
    if (timer >= 0.0f) {
        const auto remaining = timer - elapsed;
        runtime.WriteFloat(object + 0x31cu, remaining);
        if (remaining > 0.0f) return 0u;
    }
    const auto named = runtime.FindObjectByName(runtime.ReadCString(
        runtime.ReadWord(object + 0x308u)));
    if (named == 0u) return 0u;
    runtime.RemoveAnimatedObject(named);
    runtime.ResetAnimatedObject(named);
    runtime.PositionAnimatedObject(named, object + 0xacu);
    runtime.SetAnimatedObjectVisibility(named, 1u);
    runtime.WriteWord(named + 0x190u, 1u);
    runtime.OrientAnimatedObject(named, object + 0xacu);
    runtime.SetAllAtomicMaterialFlags(named, 0xffffffffu);
    runtime.AddAnimatedObject(named);
    const auto next = runtime.RandomDelay(
        runtime.ReadFloat(object + 0x314u),
        runtime.ReadFloat(object + 0x318u),
        static_cast<float>(runtime.TimeMilliseconds()));
    runtime.WriteFloat(object + 0x31cu, next);
    return 0u;
}

}  // namespace pal4
