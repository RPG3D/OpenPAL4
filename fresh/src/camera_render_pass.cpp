#include "pal4/camera_render_pass.h"

#include <cmath>

namespace pal4 {

void CameraRenderPass::Render(
    CameraRenderState& state, const std::uint32_t scene,
    CameraRenderGlobals& globals, ICameraRenderGeometryRuntime& geometry,
    ICameraRenderPassRuntime& runtime) {
    if (state.active == 0u) return;
    (void)CameraRenderGeometry::UpdateFrustum(
        state, globals.special_mode, globals.frustum_scale, geometry);
    if (!globals.render_enabled) return;
    if (globals.fog_strength != 0.0f && globals.special_mode) {
        runtime.SetProjection(state.texture, scene, state.camera,
                              globals.fog_strength, globals.frustum_scale);
        runtime.TraverseSectors(scene, kVisibilityCallback, state);
        runtime.TraceNoop();
    }
    if (!globals.render_enabled) return;
    if (globals.special_mode) {
        globals.reset_counter = 0u;
    } else if (globals.fog_strength != 0.0f) {
        runtime.RenderCollision(state.camera, state.current_raster,
                                state.world, state.maximum,
                                globals.fog_strength,
                                globals.frustum_scale,
                                globals.fog_option);
    }
}

std::uint32_t CameraRenderPass::RenderScene(
    CameraRenderState& state, const std::uint32_t unused,
    const std::uint32_t callback,
    const std::uint32_t callback_argument,
    const bool move_along_target, const CameraRenderGlobals& globals,
    ICameraRenderGeometryRuntime& geometry,
    ICameraRenderPassRuntime& runtime) {
    (void)unused;
    if (state.active == 0u) return 0u;
    (void)CameraRenderGeometry::UpdatePosition(
        state, move_along_target, geometry);
    runtime.UpdateTarget(state.camera, state.target);
    if (!globals.skip_clump_callbacks)
        runtime.ForAllAtomics(state.clump, kPreClumpCallback);
    runtime.RenderSceneInternal(state.camera, state.clump,
                                !globals.special_mode, callback,
                                callback_argument);
    if (!globals.skip_clump_callbacks)
        runtime.ForAllAtomics(state.clump, kPostClumpCallback);
    if (globals.stereo) {
        runtime.RenderStereo(state.stereo_raster, state.primary_raster,
                             state.auxiliary_camera);
        state.current_raster = state.stereo_raster;
    } else {
        state.current_raster = state.primary_raster;
    }
    if (globals.post_process_mode != 0u)
        runtime.RenderEffects(state.current_raster, state.effects_raster,
                              state.auxiliary_camera,
                              globals.post_process_mode);
    if (!globals.special_mode) return 0u;
    if (std::fabs(globals.fog_strength) < 1.0f ||
        globals.fog_option != 0u)
        return runtime.RenderFog(
            state.auxiliary_camera, state.current_raster,
            globals.fog_strength, globals.frustum_scale,
            runtime.ReadFloat(state.target + 12u), globals.fog_option);
    return globals.fog_option;
}

std::uint32_t CameraRenderPass::RemoveFromList(
    const std::uint32_t candidate, std::uint32_t& slot) {
    if (candidate == slot) {
        slot = 0u;
        return 0u;
    }
    return candidate;
}

}  // namespace pal4
