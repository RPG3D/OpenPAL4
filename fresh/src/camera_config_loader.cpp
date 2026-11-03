#include "pal4/camera_config_loader.h"

#include "pal4/camera_mode_configuration.h"
#include "pal4/camera_parameter_setters.h"

#include <bit>

namespace pal4 {

float CameraConfigLoader::TableFloat(
    const CameraControllerState& state, std::uint32_t controller_address,
    std::uint32_t address) {
    const auto offset = address - controller_address;
    if (offset <= sizeof(CameraControllerState) - sizeof(std::uint32_t) &&
        offset % 4u == 0u)
        return std::bit_cast<float>(state.At(offset));
    return std::bit_cast<float>(runtime_.ReadWord(address));
}

bool CameraConfigLoader::Load(CameraControllerState& state,
                              std::uint32_t controller_address,
                              std::uint32_t reader_argument) {
    const auto node = runtime_.CreateNode("CAM_MGR");
    bool destroyed = false;
    try {
        const auto reader = runtime_.PrepareReader(reader_argument);
        if (!runtime_.IsCompatible(reader, node)) {
            destroyed = true;
            runtime_.DestroyNode(node);
            return false;
        }

        const auto mode_attribute = runtime_.FindAttribute(
            node, "current-camera-mode");
        const auto mode = mode_attribute ?
            runtime_.ParseInt(*mode_attribute, 0u) : 0u;
        state.At(0xc0u) = mode;
        static_cast<void>(modes_.SetModeByIndex(
            state, controller_address, mode));

        const auto selected = state.At(0xbcu);
        const auto default_pitch = TableFloat(
            state, controller_address, selected + 4u);
        const auto pitch_attribute = runtime_.FindAttribute(
            node, "current-camera-PIT");
        const auto pitch = pitch_attribute ?
            runtime_.ParseFloat(*pitch_attribute, default_pitch) :
            default_pitch;

        const auto default_yaw = TableFloat(
            state, controller_address, state.At(0xbcu));
        const auto yaw_attribute = runtime_.FindAttribute(
            node, "current-camera-YAW");
        const auto yaw = yaw_attribute ?
            runtime_.ParseFloat(*yaw_attribute, default_yaw) :
            default_yaw;

        const auto& object = runtime_.ObjectAt(state.At(0x34u));
        auto& camera = runtime_.CameraDataAt(object.camera);
        static_cast<void>(parameters_.SetFirstAngle(camera, yaw, 1u));
        static_cast<void>(parameters_.SetSecondAngle(camera, pitch, 1u));
        static_cast<void>(parameters_.SetThirdAngle(camera, 0.0f, 1u));

        destroyed = true;
        runtime_.DestroyNode(node);
        return true;
    } catch (...) {
        if (!destroyed) runtime_.DestroyNode(node);
        throw;
    }
}

}  // namespace pal4
