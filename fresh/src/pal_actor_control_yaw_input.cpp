#include "pal4/pal_actor_control_yaw_input.h"

namespace pal4 {

ActorControlMouseInput IActorControlYawRuntime::ReadYawInput() {
    return {};
}
float IActorControlYawRuntime::ReadCameraYaw(std::uint32_t) {
    return 0.0f;
}
void IActorControlYawRuntime::ApplyCameraYaw(
    std::uint32_t, float, bool) {}

std::uint32_t PalActorControlYawInput::Handle(
    float delta, std::uint32_t camera_internal,
    IActorControlYawRuntime& runtime) {
    const auto yaw = runtime.ReadCameraYaw(camera_internal);
    const auto primary = runtime.ReadYawInput();
    std::uint32_t result = 0u;
    if (primary.primary_count > 0 && (primary.primary_mode & 0xfu) != 0u) {
        runtime.ApplyCameraYaw(camera_internal, yaw + delta * 200.0f, true);
        result = 1u;
    } else {
        const auto secondary = runtime.ReadYawInput();
        if (secondary.secondary_count > 0 &&
            (secondary.secondary_mode & 0xfu) != 0u) {
            runtime.ApplyCameraYaw(camera_internal,
                                   yaw - delta * 200.0f, true);
            result = 1u;
        }
    }
    const auto primary_release = runtime.ReadYawInput();
    if (primary_release.primary_count == 0 &&
        primary_release.primary_release != 0u)
        return 2u;
    const auto secondary_release = runtime.ReadYawInput();
    if (secondary_release.secondary_count == 0 &&
        secondary_release.secondary_release != 0u)
        return 2u;
    return result;
}

}  // namespace pal4
