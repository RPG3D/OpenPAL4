#include "pal4/camera_render_object.h"

namespace pal4 {

std::uint32_t CameraRenderObject::Initialize(
    CameraRenderState& state, const std::uint32_t world,
    const std::uint32_t clump, const std::uint32_t source_frame,
    const std::uint32_t dimensions, const std::uint32_t target,
    const CameraRenderOptions& options,
    ICameraRenderLifecycleRuntime& runtime) {
    state.world = world;
    state.clump = clump;
    state.source_frame = source_frame;
    state.dimensions = dimensions;
    state.target = target;

    const auto primary_mask = 1u << (options.raster_shift & 31u);
    const auto auxiliary_mask = options.stereo ? primary_mask >> 1u
                                               : primary_mask;
    state.camera = runtime.CreateCamera(primary_mask);
    if (state.camera == 0u) return 0u;
    runtime.SetFrame(state.camera, source_frame);
    runtime.SetClip(state.camera,
                    runtime.ReadFloat(dimensions + 12u) * 2.0f);
    runtime.UpdateTarget(state.camera, target);
    runtime.AddToWorld(world, state.camera);

    state.auxiliary_camera = runtime.CreateCamera(auxiliary_mask);
    if (state.auxiliary_camera == 0u) return 0u;
    state.primary_raster = runtime.CreateRaster(primary_mask);
    if (state.primary_raster == 0u) return 0u;
    runtime.SetCameraRaster(state.camera, state.primary_raster);
    if (options.stereo) {
        state.stereo_raster = runtime.CreateRaster(auxiliary_mask);
        if (state.stereo_raster == 0u) return 0u;
    }
    state.effects_raster = runtime.CreateRaster(auxiliary_mask);
    if (state.effects_raster == 0u) return 0u;
    state.active = 1u;
    return InitializeTextures(state, runtime);
}

std::uint32_t CameraRenderObject::InitializeTextures(
    CameraRenderState& state, ICameraRenderLifecycleRuntime& runtime) {
    state.second_pipeline = runtime.CreatePipeline();
    if (state.second_pipeline == 0u) return 1u;
    state.texture = runtime.CreateTexture(0u);
    if (state.texture == 0u) return 0u;
    runtime.WriteByte(state.texture + 80u, 2u);
    runtime.WriteByte(state.texture + 81u, 51u);
    return 1u;
}

std::uint32_t CameraRenderObject::Cleanup(
    CameraRenderState& state, ICameraRenderCleanupRuntime& runtime) {
    if (state.texture != 0u) {
        runtime.DetachTextureRaster(state.texture);
        runtime.DestroyTexture(state.texture);
        state.texture = 0u;
    }
    if (state.second_pipeline != 0u)
        runtime.DestroyPipeline(state.second_pipeline);
    state.second_pipeline = 0u;
    if (state.first_pipeline != 0u)
        runtime.DestroyPipeline(state.first_pipeline);
    state.first_pipeline = 0u;
    if (state.camera != 0u) {
        const auto world = runtime.GetCameraWorld(state.camera);
        if (world != 0u) runtime.RemoveFromWorld(world, state.camera);
        runtime.DestroyCamera(state.camera);
        state.camera = 0u;
    }
    if (state.auxiliary_camera != 0u) {
        runtime.DestroyCamera(state.auxiliary_camera);
        state.auxiliary_camera = 0u;
    }
    if (state.primary_raster != 0u) {
        (void)runtime.DestroyRaster(state.primary_raster);
        state.primary_raster = 0u;
    }
    if (state.stereo_raster != 0u) {
        (void)runtime.DestroyRaster(state.stereo_raster);
        state.stereo_raster = 0u;
    }
    auto result = state.effects_raster;
    if (state.effects_raster != 0u) {
        result = runtime.DestroyRaster(state.effects_raster);
        state.effects_raster = 0u;
    }
    state.world = 0u;
    state.clump = 0u;
    state.source_frame = 0u;
    state.dimensions = 0u;
    state.target = 0u;
    state.current_raster = 0u;
    state.active = 0u;
    return result;
}

}  // namespace pal4
