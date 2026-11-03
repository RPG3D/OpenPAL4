#include "pal4/camera_controller_access.h"

#include <cstring>
#include <optional>

namespace pal4 {

namespace {

std::optional<std::uint32_t> FixedCameraOffset(const char* name) {
    if (std::strcmp(name, "MainCamera") == 0) return 0x34u;
    if (std::strcmp(name, "ScriptCamera") == 0) return 0x38u;
    if (std::strcmp(name, "UiCamera") == 0) return 0x40u;
    if (std::strcmp(name, "CombatCamera") == 0) return 0x3cu;
    return std::nullopt;
}

}  // namespace

bool CameraControllerAccess::SetFarClipPlane(
    const CameraControllerState& state, float distance) {
    const auto object_address = state.At(0x30u);
    if (object_address == 0u) return false;
    const auto& object = runtime_.ObjectAt(object_address);
    runtime_.SetFarClip(object.camera, distance);
    return true;
}

std::uint32_t CameraControllerAccess::GetObjectByName(
    const CameraControllerState& state, const char* name) {
    if (const auto offset = FixedCameraOffset(name)) return state.At(*offset);
    return runtime_.FindNamedObject(state, 0x10u, name).value_or(0u);
}

bool CameraControllerAccess::SetActiveCameraByName(
    CameraControllerState& state, const char* name) {
    if (const auto offset = FixedCameraOffset(name)) {
        state.At(0x30u) = state.At(*offset);
        return true;
    }
    if (const auto object = runtime_.FindNamedObject(state, 0x10u, name)) {
        state.At(0x30u) = *object;
        return true;
    }
    runtime_.ReportMissingCameraName(name);
    return false;
}

std::uint32_t CameraControllerAccess::ActiveObject(
    const CameraControllerState& state) const {
    return state.At(0x30u);
}

std::uint32_t CameraControllerAccess::ActiveInternalId(
    const CameraControllerState& state) {
    const auto object = ActiveObject(state);
    return object == 0u ? 0u : runtime_.ObjectAt(object).camera;
}

std::uint32_t CameraControllerAccess::ActiveCameraPointer(
    const CameraControllerState& state) {
    const auto camera = ActiveInternalId(state);
    return camera == 0u ? 0u : runtime_.CameraDataAt(camera).At(0u);
}

bool CameraControllerAccess::IsMainCameraActive(
    const CameraControllerState& state) const {
    const auto main = state.At(0x34u);
    return main != 0u && state.At(0x30u) != 0u &&
           main == state.At(0x30u);
}

}  // namespace pal4
