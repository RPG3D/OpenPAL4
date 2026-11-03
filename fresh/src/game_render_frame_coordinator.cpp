#include "pal4/game_render_frame_coordinator.h"

namespace pal4 {

int GameRenderFrameCoordinator::Update(const double elapsed,
                             const std::uint32_t override_camera,
                             IGameFrameRuntime& runtime) {
    const auto mode_result = runtime.HandleVideoModeChange();
    if (mode_result == 0) return mode_result;

    const auto first_camera = runtime.ActiveCamera(runtime.InitCameraSubsystem());
    if (first_camera == 0u) return 0;
    const auto first_id_slot = runtime.InternalIdSlot(first_camera);
    const auto game = runtime.GameInstance();
    const auto object = runtime.ReadWord(game + kGameObjectOffset);
    if (object == 0u) return static_cast<int>(game);

    const auto combat_camera = runtime.CombatCamera(runtime.CombatSystem());
    const auto audio = runtime.AudioManager();
    runtime.UpdateAudio(audio, static_cast<float>(
        static_cast<double>(runtime.ReadFloat(runtime.GameState() + kScaleOffset))
        * elapsed));
    runtime.UpdateGameObject(object, static_cast<float>(
        static_cast<double>(runtime.ReadFloat(runtime.GameState() + kScaleOffset))
        * elapsed), runtime.ReadWord(first_id_slot), override_camera);

    const auto second_camera = runtime.ActiveCamera(runtime.InitCameraSubsystem());
    if (second_camera == 0u) return 0;
    const auto second_id_slot = runtime.InternalIdSlot(second_camera);
    runtime.UpdateRenderParams(combat_camera, elapsed);

    if (second_id_slot != 0u) {
        const auto frame = combat_camera + kRenderFrameOffset;
        runtime.PrepareRenderFrame(frame, runtime.CameraContainer(second_camera));
        runtime.SetRenderOption(20u, 2u);
        runtime.SetRenderOption(6u, 1u);
        runtime.SetRenderOption(8u, 1u);
        runtime.RenderGameObject(object, second_id_slot);

        if (override_camera == 0u) {
            if (runtime.ReadWord(runtime.GameState() + kTextureOverlayOffset) != 0u) {
                const auto overlay_game = runtime.GameInstance();
                runtime.DrawTexturedRectangle(
                    second_id_slot,
                    runtime.ReadWord(overlay_game + kTextureOffset),
                    544u, 400u, 256u, 200u, {255u, 255u, 255u, 255u});
            }
            if (runtime.ReadWord(runtime.GameState() + kTextOverlayOffset) != 0u)
                runtime.DrawTextCollection(runtime.RenderSystem(), second_id_slot);
        }

        runtime.EndRenderFrame(frame);
        runtime.RenderScene(frame);
        const auto camera = override_camera != 0u
            ? override_camera : runtime.ReadWord(second_id_slot);
        runtime.CameraClear(camera, kClearColor, 3u);
        const auto updated_camera = runtime.CameraBeginUpdate(camera);
        if (updated_camera != 0u) {
            runtime.RenderPasses(combat_camera, updated_camera);
            runtime.CameraEndUpdate(updated_camera);
            runtime.ShowGameRaster(updated_camera);
            runtime.ClearRenderCollection(runtime.RenderSystem());
        }
    }

    if (override_camera == 0u &&
        runtime.ReadWord(runtime.GameState() + kTextureOverlayOffset) != 0u) {
        const auto overlay_game = runtime.GameInstance();
        runtime.ApplyTextureProperties(
            overlay_game + kTexturePropertyOffset,
            static_cast<float>(elapsed), runtime.TexturePropertyGlobal());
    }
    return runtime.HandleDeviceLoss();
}

}  // namespace pal4
