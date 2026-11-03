#include "pal4/pal4_game_frame_renderer.h"

namespace pal4 {
namespace {

constexpr std::array<std::uint8_t, 4> kWhite{255u, 255u, 255u, 255u};

float ScaledElapsed(const double elapsed, IPal4GameFrameRuntime& runtime) {
    const auto state = runtime.GameState();
    const auto scale = runtime.ReadFloat(
        state + Pal4GameFrameRenderer::kScaleOffset);
    return static_cast<float>(static_cast<double>(scale) * elapsed);
}

void SetOptions(IPal4GameFrameRuntime& runtime) {
    for (const auto& [option, value] : Pal4GameFrameRenderer::kSceneRenderOptions)
        runtime.SetRenderOption(option, value);
}

void DrawOverlays(const std::uint32_t camera_id_slot,
                  IPal4GameFrameRuntime& runtime) {
    const auto texture_state = runtime.GameState();
    if (runtime.ReadWord(texture_state +
                         Pal4GameFrameRenderer::kTextureOverlayOffset) != 0u) {
        const auto game = runtime.GameInstance();
        const auto texture = runtime.ReadWord(
            game + Pal4GameFrameRenderer::kTextureOffset);
        runtime.DrawTexturedRectangle(
            camera_id_slot, texture,
            544u, 400u, 256u, 200u, kWhite);
    }
    const auto text_state = runtime.GameState();
    if (runtime.ReadWord(text_state +
                         Pal4GameFrameRenderer::kTextOverlayOffset) != 0u) {
        const auto system = runtime.RenderSystem();
        runtime.DrawTextCollection(system, camera_id_slot);
    }
}

void ApplyTextureOverlay(const double elapsed,
                         IPal4GameFrameRuntime& runtime) {
    const auto state = runtime.GameState();
    if (runtime.ReadWord(state +
                         Pal4GameFrameRenderer::kTextureOverlayOffset) != 0u) {
        const auto game = runtime.GameInstance();
        const auto callback = runtime.FrameCallback();
        runtime.ApplyTextureProperties(
            game + Pal4GameFrameRenderer::kTextureObjectOffset,
            static_cast<float>(elapsed), callback);
    }
}

}  // namespace

std::int32_t Pal4GameFrameRenderer::Render(
    const double elapsed, const std::uint32_t override_camera,
    IPal4GameFrameRuntime& runtime) {
    const auto mode = runtime.HandleVideoModeChange();
    if (mode == 0) return mode;
    const auto first_subsystem = runtime.InitCameraSubsystem();
    const auto first_camera = runtime.ActiveCamera(first_subsystem);
    if (first_camera == 0u) return 0;
    const auto first_id_slot = runtime.InternalIdSlot(first_camera);
    const auto game = runtime.GameInstance();
    const auto object = runtime.ReadWord(game + kGameObjectOffset);
    if (object == 0u) return static_cast<std::int32_t>(game);

    const auto audio_manager = runtime.AudioManager();
    const auto audio_elapsed = ScaledElapsed(elapsed, runtime);
    runtime.UpdateAudio(audio_manager, audio_elapsed);
    const auto object_state = runtime.GameState();
    const auto first_id = runtime.ReadWord(first_id_slot);
    const auto object_scale = runtime.ReadFloat(object_state + kScaleOffset);
    const auto object_elapsed = static_cast<float>(
        static_cast<double>(object_scale) * elapsed);
    runtime.UpdateGameObject(object, object_elapsed, first_id,
                             override_camera);
    const auto updated_game = runtime.GameInstance();
    if (runtime.ReadWord(updated_game + kGameObjectOffset) != object)
        return static_cast<std::int32_t>(updated_game);

    const auto second_subsystem = runtime.InitCameraSubsystem();
    const auto second_camera = runtime.ActiveCamera(second_subsystem);
    if (second_camera == 0u) return 0;
    const auto id_slot = runtime.InternalIdSlot(second_camera);
    if (id_slot != 0u) {
        const auto clear_camera = override_camera != 0u ?
                                      override_camera : runtime.ReadWord(id_slot);
        runtime.CameraClear(clear_camera, kClearColor, 3u);
        const auto begin_camera = override_camera != 0u ?
                                      override_camera : runtime.ReadWord(id_slot);
        const auto updated_camera = runtime.CameraBeginUpdate(begin_camera);
        if (updated_camera != 0u) {
            SetOptions(runtime);
            runtime.RenderGameObject(object, id_slot);
            if (override_camera == 0u) DrawOverlays(id_slot, runtime);
            runtime.CameraEndUpdate(updated_camera);
            runtime.ShowGameRaster(updated_camera);
            const auto system = runtime.RenderSystem();
            runtime.ClearRenderCollection(system);
        }
    }
    if (override_camera == 0u) ApplyTextureOverlay(elapsed, runtime);
    return runtime.HandleDeviceLoss();
}

std::int32_t Pal4GameFrameRenderer::RenderWithEffects(
    const double elapsed, const std::uint32_t override_camera,
    IPal4GameFrameRuntime& runtime) {
    const auto mode = runtime.HandleVideoModeChange();
    if (mode == 0) return mode;
    const auto first_subsystem = runtime.InitCameraSubsystem();
    const auto first_camera = runtime.ActiveCamera(first_subsystem);
    if (first_camera == 0u) return 0;
    const auto first_id_slot = runtime.InternalIdSlot(first_camera);
    const auto game = runtime.GameInstance();
    const auto object = runtime.ReadWord(game + kGameObjectOffset);
    if (object == 0u) return static_cast<std::int32_t>(game);

    const auto audio_manager = runtime.AudioManager();
    const auto audio_elapsed = ScaledElapsed(elapsed, runtime);
    runtime.UpdateAudio(audio_manager, audio_elapsed);
    const auto object_state = runtime.GameState();
    const auto first_id = runtime.ReadWord(first_id_slot);
    const auto object_scale = runtime.ReadFloat(object_state + kScaleOffset);
    const auto object_elapsed = static_cast<float>(
        static_cast<double>(object_scale) * elapsed);
    runtime.UpdateGameObject(object, object_elapsed, first_id,
                             override_camera);
    const auto second_subsystem = runtime.InitCameraSubsystem();
    const auto second_camera = runtime.ActiveCamera(second_subsystem);
    if (second_camera == 0u) return 0;
    const auto id_slot = runtime.InternalIdSlot(second_camera);
    const auto update_manager = runtime.EffectManager();
    runtime.UpdateRenderParams(update_manager, elapsed);
    if (id_slot != 0u) {
        const auto frame = runtime.EffectManager() + kEffectFrameOffset;
        const auto container = runtime.CameraContainer(second_camera);
        runtime.PrepareRenderFrame(frame, container);
        SetOptions(runtime);
        runtime.RenderGameObject(object, id_slot);
        if (override_camera == 0u) DrawOverlays(id_slot, runtime);
        const auto end_frame = runtime.EffectManager() + kEffectFrameOffset;
        runtime.EndRenderFrame(end_frame);
        const auto scene_frame = runtime.EffectManager() + kEffectFrameOffset;
        runtime.RenderScene(scene_frame);
        const auto clear_camera = override_camera != 0u ?
                                      override_camera : runtime.ReadWord(id_slot);
        runtime.CameraClear(clear_camera, kClearColor, 3u);
        const auto begin_camera = override_camera != 0u ?
                                      override_camera : runtime.ReadWord(id_slot);
        const auto updated_camera = runtime.CameraBeginUpdate(begin_camera);
        if (updated_camera != 0u) {
            const auto pass_manager = runtime.EffectManager();
            runtime.RenderEffectPasses(pass_manager, updated_camera);
            runtime.CameraEndUpdate(updated_camera);
            runtime.ShowGameRaster(updated_camera);
            const auto system = runtime.RenderSystem();
            runtime.ClearRenderCollection(system);
        }
    }
    if (override_camera == 0u) ApplyTextureOverlay(elapsed, runtime);
    return runtime.HandleDeviceLoss();
}

}  // namespace pal4
