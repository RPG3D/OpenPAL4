#include "pal4/camera_system_object.h"
#include "pal4/camera_allocation.h"
#include "pal4/camera_parameter_setters.h"

#include <bit>

namespace pal4 {

CameraSystemObjectState& CameraSystemObject::Construct(
    CameraSystemObjectState& state) {
    state.camera = 0u;
    runtime_.InitializeName(state.name);
    runtime_.InitializeTasks(state.tasks);
    return state;
}

std::uint32_t CameraSystemObject::Destroy(
    CameraSystemObjectState& state) {
    static_cast<void>(allocation_.Release(state.camera));
    runtime_.DestroyTasks(state.tasks);
    return runtime_.CleanupName(state.name);
}

std::uint32_t CameraSystemObject::SetSceneParameter(
    CameraSystemObjectState& state, std::uint32_t parameter) {
    runtime_.CameraData(state.camera).At(0x34u) = parameter;
    return parameter;
}

bool CameraSystemObject::SetLookAtFromScene(
    CameraSystemObjectState& state, std::uint32_t selector) {
    auto& camera = runtime_.CameraData(state.camera);
    const auto scene = camera.At(0x34u);
    if (scene == 0u) return false;
    const auto target = runtime_.SceneLookAt(scene, selector);
    static_cast<void>(parameters_.SetLookAt(camera, target, selector));
    return true;
}

std::uint32_t CameraSystemObject::InternalId(
    const CameraSystemObjectState& state) const {
    return state.camera;
}

std::optional<std::uint32_t> CameraSystemObject::Update(
    CameraSystemObjectState& state, CameraAnimation& animation,
    float delta) {
    if (state.camera == 0u) return std::nullopt;
    runtime_.UpdateTasks(state.tasks, delta);
    return animation.Update(runtime_.CameraData(state.camera), delta);
}

std::uint32_t CameraSystemObject::AdjustAngles(
    CameraSystemObjectState& state,
    const std::array<float, 3>& delta,
    std::uint32_t mode) {
    auto& camera = runtime_.CameraData(state.camera);
    return parameters_.AdjustAngles(camera, delta, mode);
}

std::optional<std::uint32_t> CameraSystemObject::AdjustDistance(
    CameraSystemObjectState& state, float delta,
    std::uint32_t mode) {
    auto& camera = runtime_.CameraData(state.camera);
    const auto current = std::bit_cast<float>(camera.At(0x48u));
    return parameters_.SetDistance(camera, current + delta, mode);
}

std::uint32_t CameraSystemObject::AdjustAnglesAndDistance(
    CameraSystemObjectState& state,
    const std::array<float, 3>& angle_delta,
    float distance_delta, std::uint32_t mode) {
    auto& camera = runtime_.CameraData(state.camera);
    std::array<float, 3> angles{};
    for (std::size_t axis = 0; axis < angles.size(); ++axis)
        angles[axis] = std::bit_cast<float>(
            camera.At(0x3cu + axis * 4u)) + angle_delta[axis];
    const auto distance = std::bit_cast<float>(camera.At(0x48u)) +
                          distance_delta;
    return parameters_.SetAnglesAndDistance(camera, angles, distance, mode);
}

}  // namespace pal4
