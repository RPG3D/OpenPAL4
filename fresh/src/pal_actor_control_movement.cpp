#include "pal4/pal_actor_control_movement.h"
#include "pal4/pal_actor_control_transform.h"

#include <bit>
#include <cmath>

namespace pal4 {

float IActorControlMovementRuntime::ComputeTransform(
    PalActorControlState& state, std::uint32_t camera,
    std::uint32_t actor, float& axis_y) {
    ActorControlTransformRuntimeStub runtime;
    return PalActorControlTransform::Compute(state, camera, actor,
                                             &axis_y, runtime);
}
float IActorControlMovementRuntime::CameraYaw(std::uint32_t) {
    return 0.0f;
}
void IActorControlMovementRuntime::SetCameraYaw(
    std::uint32_t, float, std::uint32_t) {}

std::uint32_t PalActorControlMovement::UpdateYaw(
    PalActorControlState& state, std::uint32_t camera,
    std::uint32_t actor, float step,
    IActorControlMovementRuntime& runtime) {
    auto& component = state.words[5];
    const auto old_axis_y = std::bit_cast<float>(component);
    float axis_y = old_axis_y;
    const auto magnitude = runtime.ComputeTransform(state, camera, actor,
                                                     axis_y);
    component = std::bit_cast<std::uint32_t>(axis_y);
    const auto yaw = runtime.CameraYaw(camera);

    constexpr double kSignChangeThreshold =
        std::bit_cast<double>(0x3f747ae140000000ull);
    const bool crossed_zero =
        std::fabs(static_cast<double>(old_axis_y)) >
            kSignChangeThreshold &&
        ((old_axis_y < 0.0f && axis_y >= 0.0f) ||
         (old_axis_y > 0.0f && axis_y <= 0.0f));
    if (magnitude == 0.0f)
        return 0u;

    if (axis_y < 0.0f) {
        if (!crossed_zero && magnitude > step) {
            runtime.SetCameraYaw(camera, yaw + step, 1u);
            return 1u;
        }
        runtime.SetCameraYaw(camera, yaw + magnitude, 1u);
    } else if (axis_y > 0.0f) {
        if (!crossed_zero && magnitude > step) {
            runtime.SetCameraYaw(camera, yaw - step, 1u);
            return 1u;
        }
        runtime.SetCameraYaw(camera, yaw - magnitude, 1u);
    } else if (!crossed_zero) {
        return 0u;
    }

    component = 0u;
    return 0u;
}

}  // namespace pal4
