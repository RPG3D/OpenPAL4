#include "pal4/combat_camera_access.h"

namespace pal4 {

namespace {
constexpr std::string_view kCombatCameraName = "CombatCamera";
}

std::uint32_t CombatCameraAccess::Setup(ICombatCameraRuntime& runtime) {
    const auto camera_object = runtime.FindCamera(
        runtime.CameraManager(), kCombatCameraName);
    if (camera_object == 0u) return 0u;

    runtime.SetPosition(runtime.InternalCamera(camera_object),
                        {13.0f, 20.0f, -2.0f}, 1u);
    runtime.SetDistance(runtime.InternalCamera(camera_object), 452.0f, 1u);
    return runtime.SetAngles(runtime.InternalCamera(camera_object),
                             {178.0f, 14.0f, 0.0f}, 1u);
}

std::uint32_t CombatCameraAccess::Current(ICombatCameraRuntime& runtime) {
    const auto camera_object = runtime.FindCamera(
        runtime.CameraManager(), kCombatCameraName);
    if (camera_object == 0u) return 0u;
    return runtime.InternalCamera(camera_object);
}

}  // namespace pal4
