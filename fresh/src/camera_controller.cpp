#include "pal4/camera_controller.h"

namespace pal4 {

CameraControllerState& CameraController::Construct(
    CameraControllerState& state) {
    state.At(0x4u) = 0u;
    state.At(0x8u) = 0u;
    state.At(0xcu) = 0u;

    state.At(0x10u) = 0u;
    state.At(0x14u) = 0u;
    state.At(0x20u) = 0u;
    state.At(0x24u) = 0u;
    state.At(0x28u) = (state.At(0x28u) & 0xff000000u) | 0x00010100u;
    state.At(0x2cu) = 0u;

    state.At(0x30u) = 0u;
    state.At(0xbcu) = 0u;
    state.At(0x34u) = 0u;
    state.At(0x38u) = 0u;
    state.At(0x3cu) = 0u;
    state.At(0x40u) = 0u;
    runtime_.SetName(state.At(0x2cu), "palCameraController set");

    state.At(0x14u) = 0x5eaea0u;
    state.At(0x10u) = 0x5eaec0u;
    state.At(0x18u) = 0x5eaf50u;
    state.At(0x1cu) = 0x5fc6c0u;
    return state;
}

std::uint32_t CameraController::DestroyComponent(
    CameraControllerState& state) {
    state.At(0x30u) = 0u;
    state.At(0xbcu) = 0u;
    runtime_.DestroyNameObject(state, 0x10u);
    const auto result = runtime_.FreeData(state.At(0x4u));
    state.At(0x4u) = 0u;
    state.At(0x8u) = 0u;
    state.At(0xcu) = 0u;
    return result;
}

std::uint32_t CameraController::ClearDynamicCollection(
    CameraControllerState& state) {
    const auto old_end = state.At(0x8u);
    state.At(0x8u) = state.At(0x4u);
    return old_end;
}

}  // namespace pal4
