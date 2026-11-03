#include "pal4/camera_config_persistence.h"

#include <bit>

namespace pal4 {

std::uint8_t CameraConfigPersistence::Save(
    const CameraControllerState& state, std::uint32_t writer_argument) {
    const auto node = runtime_.CreateNode("CAM_MGR");
    bool destroyed = false;
    try {
        constexpr std::string_view mode_key = "current-camera-mode";
        const auto mode = state.At(0xc0u);
        if (runtime_.HasAttribute(node, mode_key))
            runtime_.SetInt(node, mode_key, mode);
        else
            runtime_.AddInt(node, mode_key, mode);

        const auto& object = runtime_.ObjectAt(state.At(0x34u));
        const auto& camera = runtime_.CameraDataAt(object.camera);
        constexpr std::string_view yaw_key = "current-camera-YAW";
        const auto yaw = std::bit_cast<float>(camera.At(0x3cu));
        if (runtime_.HasAttribute(node, yaw_key))
            runtime_.SetFloat(node, yaw_key, yaw);
        else
            runtime_.AddFloat(node, yaw_key, yaw);

        constexpr std::string_view pitch_key = "current-camera-PIT";
        const auto pitch = std::bit_cast<float>(camera.At(0x40u));
        if (runtime_.HasAttribute(node, pitch_key))
            runtime_.SetFloat(node, pitch_key, pitch);
        else
            runtime_.AddFloat(node, pitch_key, pitch);

        const auto writer = runtime_.PrepareWriter(writer_argument);
        const auto result = runtime_.Write(node, writer);
        destroyed = true;
        runtime_.DestroyNode(node);
        return result;
    } catch (...) {
        if (!destroyed) runtime_.DestroyNode(node);
        throw;
    }
}

}  // namespace pal4
