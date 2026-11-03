#include "pal4/camera_scene_frame.h"

namespace pal4 {

std::uint32_t CameraSceneFrame::RenderInternal(
    const std::uint32_t camera, const std::uint32_t clump,
    const bool draw_quad, const std::uint32_t callback,
    const std::uint32_t callback_argument,
    ICameraSceneFrameRuntime& runtime) {
    runtime.ClearCamera(camera, {0xffu, 0xffu, 0xffu, 0u}, 3u);
    if (runtime.BeginUpdate(camera) != 0u) {
        if (callback != 0u)
            runtime.InvokeCallback(callback, callback_argument);
        runtime.RenderClump(clump);
        if (draw_quad) (void)DrawQuad(camera, runtime);
        runtime.EndUpdate(camera);
    }
    return camera;
}

std::uint32_t CameraSceneFrame::DrawQuad(
    const std::uint32_t camera, ICameraSceneFrameRuntime& runtime) {
    const auto raster = runtime.ReadWord(camera + 96u);
    const auto width = static_cast<float>(
        runtime.ReadSignedWord(raster + 12u));
    const auto height = static_cast<float>(
        runtime.ReadSignedWord(raster + 16u));
    const auto reciprocal_w = 1.0f / runtime.ReadFloat(camera + 128u);
    const auto depth = runtime.DeviceDepth();

    std::array<CameraScreenVertex, 4u> vertices{};
    vertices[0] = {0.0f, 0.0f, depth, reciprocal_w, 0xffffffffu};
    vertices[1] = {0.0f, height, depth, reciprocal_w, 0xffffffffu};
    vertices[2] = {width, 0.0f, depth, reciprocal_w, 0xffffffffu};
    vertices[3] = {width, height, depth, reciprocal_w, 0xffffffffu};

    (void)runtime.SetDeviceState(6u, 0u);
    (void)runtime.SetDeviceState(8u, 0u);
    (void)runtime.SetDeviceState(1u, 0u);
    (void)runtime.SetDeviceState(12u, 1u);
    (void)runtime.SetDeviceState(11u, 1u);
    (void)runtime.SetDeviceState(10u, 10u);
    runtime.DrawScreenQuad(4u, vertices, 4u);
    (void)runtime.SetDeviceState(6u, 1u);
    (void)runtime.SetDeviceState(8u, 1u);
    (void)runtime.SetDeviceState(11u, 6u);
    return runtime.SetDeviceState(10u, 5u);
}

}  // namespace pal4
