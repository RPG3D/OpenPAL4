#include "pal4/player_manager_slot_camera_audio.h"

#include <bit>

namespace pal4 {

void PlayerManagerSlotCameraAudioRuntimeStub::SetBaseProperty(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerSlotCameraAudioRuntimeStub::ReadDword(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotCameraAudioRuntimeStub::CameraManager() {
    return 0u;
}
std::uint32_t PlayerManagerSlotCameraAudioRuntimeStub::ActiveCameraObject(
    std::uint32_t) { return 0u; }
void PlayerManagerSlotCameraAudioRuntimeStub::SetLookAt(
    std::uint32_t, std::uint32_t) {}
std::uint32_t PlayerManagerSlotCameraAudioRuntimeStub::ActiveCameraInternal(
    std::uint32_t) { return 0u; }
std::uint32_t PlayerManagerSlotCameraAudioRuntimeStub::AudioManager() {
    return 0u;
}
std::uint32_t PlayerManagerSlotCameraAudioRuntimeStub::SetListenerPosition(
    std::uint32_t, float, float, float) { return 0u; }
std::uint32_t PlayerManagerSlotCameraAudioRuntimeStub::SetListenerOrientation(
    std::uint32_t, float, float, float, float, float, float) {
    return 0u;
}

std::uint32_t PlayerManagerSlotCameraAudio::Update(
    std::uint32_t object, std::uint32_t property,
    IPlayerManagerSlotCameraAudioRuntime& runtime) {
    runtime.SetBaseProperty(object, property);
    const auto enabled = runtime.ReadDword(object + 0xb0cu);
    if (enabled == 0u) return enabled;

    const auto first_manager = runtime.CameraManager();
    runtime.SetLookAt(runtime.ActiveCameraObject(first_manager), 1u);
    const auto second_manager = runtime.CameraManager();
    const auto first_camera = runtime.ActiveCameraInternal(second_manager);
    if (first_camera == 0u) return first_camera;

    const auto third_manager = runtime.CameraManager();
    const auto position_camera = runtime.ActiveCameraInternal(third_manager);
    const auto position_x = std::bit_cast<float>(
        runtime.ReadDword(position_camera + 0x74u));
    const auto position_y = std::bit_cast<float>(
        runtime.ReadDword(position_camera + 0x78u));
    const auto position_z = std::bit_cast<float>(
        runtime.ReadDword(position_camera + 0x7cu));
    const auto first_audio = runtime.AudioManager();
    const auto listener = runtime.ReadDword(first_audio + 0x1cu);
    static_cast<void>(runtime.SetListenerPosition(
        listener, position_x, position_y, -position_z));

    const auto fourth_manager = runtime.CameraManager();
    const auto orientation_camera = runtime.ActiveCameraInternal(
        fourth_manager);
    const auto forward_x = std::bit_cast<float>(
        runtime.ReadDword(orientation_camera + 0x50u));
    const auto forward_y = std::bit_cast<float>(
        runtime.ReadDword(orientation_camera + 0x54u));
    const auto forward_z = std::bit_cast<float>(
        runtime.ReadDword(orientation_camera + 0x58u));
    const auto second_audio = runtime.AudioManager();
    const auto second_listener = runtime.ReadDword(second_audio + 0x1cu);
    return runtime.SetListenerOrientation(
        second_listener, forward_x, forward_y, -forward_z,
        0.0f, 1.0f, -0.0f);
}

}  // namespace pal4
