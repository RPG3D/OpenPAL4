#include "pal4/screen_effect_camera.h"

namespace pal4 {

std::int32_t ScreenEffectCameraRuntimeStub::ScreenWidth() { return 0; }
std::int32_t ScreenEffectCameraRuntimeStub::ScreenHeight() { return 0; }
std::uint32_t ScreenEffectCameraRuntimeStub::CreateRaster(
    std::int32_t, std::int32_t, std::uint32_t, std::uint32_t) { return 0u; }
void ScreenEffectCameraRuntimeStub::DestroyRaster(std::uint32_t) {}
std::uint32_t ScreenEffectCameraRuntimeStub::CreateCamera() { return 0u; }
void ScreenEffectCameraRuntimeStub::SetCameraRaster(
    std::uint32_t, std::uint32_t, std::uint32_t) {}
std::uint32_t ScreenEffectCameraRuntimeStub::CameraRaster(
    std::uint32_t, std::uint32_t) { return 0u; }
std::uint32_t ScreenEffectCameraRuntimeStub::CreateFrame() { return 0u; }
void ScreenEffectCameraRuntimeStub::AttachFrame(
    std::uint32_t, std::uint32_t) {}
float ScreenEffectCameraRuntimeStub::GameNearClip() { return 0.0f; }
float ScreenEffectCameraRuntimeStub::GameFarClip() { return 0.0f; }
void ScreenEffectCameraRuntimeStub::SetNearClip(std::uint32_t, float) {}
void ScreenEffectCameraRuntimeStub::SetFarClip(std::uint32_t, float) {}
std::array<float, 2> ScreenEffectCameraRuntimeStub::ActiveViewWindow() {
    return {};
}
void ScreenEffectCameraRuntimeStub::SetViewWindow(
    std::uint32_t, const std::array<float, 2>&) {}
std::uint32_t ScreenEffectCameraRuntimeStub::CreateTexture(
    std::uint32_t) { return 0u; }
void ScreenEffectCameraRuntimeStub::ClearCamera(
    std::uint32_t, const std::uint32_t&, std::uint32_t) {}
std::uint32_t ScreenEffectCameraRuntimeStub::EndUpdate(
    std::uint32_t) { return 0u; }
std::uint32_t ScreenEffectCameraRuntimeStub::ActiveGameCamera() { return 0u; }
std::uint32_t ScreenEffectCameraRuntimeStub::WorldForCamera(
    std::uint32_t) { return 0u; }
void ScreenEffectCameraRuntimeStub::RemoveCamera(
    std::uint32_t, std::uint32_t) {}
void ScreenEffectCameraRuntimeStub::AddCamera(
    std::uint32_t, std::uint32_t) {}
void ScreenEffectCameraRuntimeStub::DestroyTexture(std::uint32_t) {}
void ScreenEffectCameraRuntimeStub::DestroyCamera(std::uint32_t) {}
ScreenEffectCameraRecord* ScreenEffectCameraRuntimeStub::AllocateRecord(
    std::uint32_t) { return nullptr; }
std::uint32_t ScreenEffectCameraRuntimeStub::FreeRecord(
    ScreenEffectCameraRecord*) { return 0u; }

std::uint32_t ScreenEffectCamera::EndUpdate(
    const ScreenEffectCameraRecord& state, std::uint32_t incoming_eax,
    IScreenEffectCameraRuntime& runtime) {
    if (state.active != 0u) return runtime.EndUpdate(state.camera);
    return incoming_eax;
}

std::uint8_t ScreenEffectCamera::Initialize(
    ScreenEffectCameraRecord& state, IScreenEffectCameraRuntime& runtime) {
    Cleanup(state, runtime);
    state.width = runtime.ScreenWidth();
    state.height = runtime.ScreenHeight();
    state.clear_color = 0xff000000u;
    const auto color_raster = runtime.CreateRaster(
        state.width, state.height, 0u, 5u);
    const auto depth_raster = runtime.CreateRaster(
        state.width, state.height, 0u, 1u);
    if (color_raster == 0u) {
        if (depth_raster != 0u) runtime.DestroyRaster(depth_raster);
        return 0u;
    }
    if (depth_raster == 0u) {
        runtime.DestroyRaster(color_raster);
        return 0u;
    }
    const auto camera = runtime.CreateCamera();
    runtime.SetCameraRaster(camera, 96u, color_raster);
    runtime.SetCameraRaster(camera, 100u, depth_raster);
    const auto frame = runtime.CreateFrame();
    runtime.AttachFrame(camera, frame);
    const auto near_clip = runtime.GameNearClip();
    const auto far_clip = runtime.GameFarClip();
    runtime.SetNearClip(camera, near_clip);
    runtime.SetFarClip(camera, far_clip);
    state.camera = camera;
    runtime.SetViewWindow(camera, runtime.ActiveViewWindow());
    const auto texture = runtime.CreateTexture(
        runtime.CameraRaster(state.camera, 96u));
    runtime.ClearCamera(state.camera, state.clear_color, 3u);
    state.texture = texture;
    state.active = 1u;
    return 1u;
}

void ScreenEffectCamera::Cleanup(
    ScreenEffectCameraRecord& state, IScreenEffectCameraRuntime& runtime) {
    if (state.active == 0u) return;
    state.active = 0u;
    const auto world = runtime.WorldForCamera(state.camera);
    if (world != 0u) runtime.RemoveCamera(world, state.camera);
    if (state.texture != 0u) {
        runtime.DestroyTexture(state.texture);
        state.texture = 0u;
    }
    if (runtime.CameraRaster(state.camera, 96u) != 0u) {
        runtime.SetCameraRaster(state.camera, 96u, 0u);
    }
    const auto depth_raster = runtime.CameraRaster(state.camera, 100u);
    if (depth_raster != 0u) {
        runtime.DestroyRaster(depth_raster);
        runtime.SetCameraRaster(state.camera, 100u, 0u);
    }
    if (state.camera != 0u) {
        runtime.DestroyCamera(state.camera);
        state.camera = 0u;
    }
}

void ScreenEffectCamera::SyncWorld(
    const ScreenEffectCameraRecord& state,
    IScreenEffectCameraRuntime& runtime) {
    if (state.active == 0u) return;
    const auto active_camera = runtime.ActiveGameCamera();
    if (active_camera == 0u || state.camera == 0u) return;
    const auto active_world = runtime.WorldForCamera(active_camera);
    const auto old_world = runtime.WorldForCamera(state.camera);
    if (active_world == old_world) return;
    if (old_world != 0u) runtime.RemoveCamera(old_world, state.camera);
    if (active_world != 0u) runtime.AddCamera(active_world, state.camera);
}

ScreenEffectCameraRecord* ScreenEffectCamera::Allocate(
    IScreenEffectCameraRuntime& runtime) {
    auto* state = runtime.AllocateRecord(24u);
    if (state == nullptr) return nullptr;
    state->active = 0u;
    state->texture = 0u;
    state->camera = 0u;
    state->width = 0;
    state->height = 0;
    return state;
}

std::uint32_t ScreenEffectCamera::Destroy(
    ScreenEffectCameraRecord* state, std::uint32_t incoming_eax,
    IScreenEffectCameraRuntime& runtime) {
    if (state == nullptr) return incoming_eax;
    Cleanup(*state, runtime);
    return runtime.FreeRecord(state);
}

}  // namespace pal4
