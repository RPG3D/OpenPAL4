#include "pal4/script_camera_callbacks.h"

#include "pal4/player_manager_object.h"

namespace pal4 {

void ScriptCameraCallbacks::SetCollision(
    std::uint8_t enabled, IScriptCameraCallbackRuntime& runtime) {
    for (const auto name : {"MainCamera", "ScriptCamera"}) {
        const auto camera = runtime.CameraByName(runtime.CameraSubsystem(), name);
        if (camera != 0u)
            runtime.WriteWord(runtime.CameraInternalId(camera) + 0x1a8u,
                              enabled == 1u ? 1u : 0u);
    }
}

std::uint32_t ScriptCameraCallbacks::SeekToPlayer(
    IScriptCameraCallbackRuntime& runtime) {
    const auto object = runtime.ActiveCameraObject(runtime.CameraSubsystem());
    if (object == 0u) return object;
    runtime.SetLookAt(object, 1u);

    const auto position = runtime.ActiveCameraInternalId(
        runtime.CameraSubsystem()) + 0x74u;
    const auto x = runtime.ReadFloat(position);
    const auto y = runtime.ReadFloat(position + 4u);
    const auto z = runtime.ReadFloat(position + 8u);
    const auto position_audio = runtime.AudioManager();
    runtime.SetListenerPosition(runtime.ReadWord(position_audio + 0x1cu),
                                x, y, -z);

    const auto direction = runtime.ActiveCameraInternalId(
        runtime.CameraSubsystem()) + 0x50u;
    const auto at_x = runtime.ReadFloat(direction);
    const auto at_y = runtime.ReadFloat(direction + 4u);
    const auto at_z = runtime.ReadFloat(direction + 8u);
    const auto orientation_audio = runtime.AudioManager();
    return runtime.SetListenerOrientation(
        runtime.ReadWord(orientation_audio + 0x1cu),
        at_x, at_y, -at_z, 0.0f, 1.0f, -0.0f);
}

std::uint32_t ScriptCameraCallbacks::AutoSeek(
    std::uint32_t enabled, IScriptCameraCallbackRuntime& runtime) {
    return PlayerManagerObject::SetPlayerState(
        runtime.PlayerManagerState(), enabled & 0xffu);
}

}  // namespace pal4
