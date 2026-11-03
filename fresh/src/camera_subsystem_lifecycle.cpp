#include "pal4/camera_subsystem_lifecycle.h"

namespace pal4 {

std::uint32_t CameraSubsystemLifecycle::Register(
    CameraSubsystemRegistrationState& state) {
    if ((state.guard & 1u) != 0u) return state.instance;
    state.guard = static_cast<std::uint8_t>(state.guard | 1u);
    const auto manager = runtime_.SingletonManager();
    state.instance = runtime_.Register(
        manager, "palCameraManager", 1002u, 0x5eae20u, 0x5eae80u);
    return state.instance;
}

std::uint32_t CameraSubsystemLifecycle::Create() {
    const auto memory = runtime_.Allocate(0xc4u);
    if (memory == 0u) return 0u;
    static_cast<void>(controller_.Construct(runtime_.ControllerAt(memory)));
    return memory;
}

std::optional<std::uint32_t> CameraSubsystemLifecycle::Destroy(
    std::uint32_t instance) {
    if (instance == 0u) return std::nullopt;
    static_cast<void>(controller_.DestroyComponent(
        runtime_.ControllerAt(instance)));
    return runtime_.Free(instance);
}

std::optional<std::uint32_t> CameraSubsystemLifecycle::DestroyCameraObject(
    std::uint32_t camera_object) {
    if (camera_object == 0u) return std::nullopt;
    runtime_.CleanupCameraObject(camera_object);
    return runtime_.Free(camera_object);
}

}  // namespace pal4
