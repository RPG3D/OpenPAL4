#include "pal4/camera_mode_configuration.h"
#include "pal4/camera_controller_access.h"
#include "pal4/camera_parameter_setters.h"

#include <array>
#include <bit>

namespace pal4 {

std::uint32_t CameraModeConfiguration::Configure(
    CameraControllerState& state, std::uint32_t controller_address) {
    static_cast<void>(access_.SetActiveCameraByName(state, "MainCamera"));
    state.At(0xc0u) = 0u;
    state.At(0xbcu) = controller_address + 0x44u;

    const auto first_angle = std::bit_cast<float>(state.At(0x44u));
    const auto second_angle = std::bit_cast<float>(state.At(0x48u));
    const auto distance = std::bit_cast<float>(state.At(0x4cu));
    const auto flag = state.At(0x50u);
    constexpr std::array<float, 3> zero_target{};

    for (std::uint32_t offset : {0x34u, 0x38u}) {
        const auto object_address = state.At(offset);
        auto& object = runtime_.ObjectAt(object_address);
        auto& camera = runtime_.CameraDataAt(object.camera);
        static_cast<void>(parameters_.SetActiveState(camera, 0u));
        static_cast<void>(parameters_.SetLookAt(camera, zero_target, 1u));
        static_cast<void>(parameters_.SetFirstAngle(camera, first_angle, 1u));
        static_cast<void>(parameters_.SetSecondAngle(camera, second_angle, 1u));
        static_cast<void>(parameters_.SetThirdAngle(camera, 0.0f, 1u));
        static_cast<void>(parameters_.SetDistance(camera, distance, 1u));
        camera.At(0x1a8u) = flag;
        if (offset == 0x34u) camera.At(0x2bcu) = 0x5ec270u;
        runtime_.ClearObjectCollection(object_address, 0x10u);
    }

    return CameraController::ClearDynamicCollection(state);
}

bool CameraModeConfiguration::SetModeByIndex(
    CameraControllerState& state, std::uint32_t controller_address,
    std::uint32_t index) {
    if (index >= 6u) return false;
    const auto entry_offset = 0x44u + 20u * index;
    state.At(0xc0u) = index;
    state.At(0xbcu) = controller_address + entry_offset;

    const auto first_angle = std::bit_cast<float>(state.At(entry_offset));
    const auto second_angle = std::bit_cast<float>(state.At(entry_offset + 4u));
    const auto distance = std::bit_cast<float>(state.At(entry_offset + 8u));
    const auto flag = state.At(entry_offset + 12u);
    auto& object = runtime_.ObjectAt(state.At(0x34u));
    auto& camera = runtime_.CameraDataAt(object.camera);
    static_cast<void>(parameters_.SetFirstAngle(camera, first_angle, 1u));
    static_cast<void>(parameters_.SetSecondAngle(camera, second_angle, 1u));
    static_cast<void>(parameters_.SetThirdAngle(camera, 0.0f, 1u));
    static_cast<void>(parameters_.SetDistance(camera, distance, 1u));
    camera.At(0x1a8u) = flag;
    return true;
}

}  // namespace pal4
