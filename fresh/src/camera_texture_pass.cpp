#include "pal4/camera_texture_pass.h"

namespace pal4 {

std::uint32_t CameraTexturePass::CreateSquareRaster(
    const std::uint32_t size, ICameraTexturePassRuntime& runtime) {
    return runtime.CreateRaster(size, size, 0u, 5u);
}

std::uint32_t CameraTexturePass::RenderToTexture(
    const std::uint32_t raster, const std::uint32_t texture,
    const std::uint32_t camera, ICameraTexturePassRuntime& runtime) {
    const auto size = static_cast<float>(
        runtime.ReadSignedWord(raster + 12u));
    runtime.WriteWord(camera + 96u, raster);
    const auto reciprocal_w = 1.0f / runtime.ReadFloat(camera + 132u);
    if (runtime.BeginUpdate(camera) != 0u) {
        (void)runtime.SetDeviceState(10u, 2u);
        (void)runtime.SetDeviceState(11u, 1u);
        (void)runtime.SetDeviceState(6u, 0u);
        (void)runtime.SetDeviceState(8u, 0u);
        (void)runtime.SetDeviceState(9u, 2u);
        (void)runtime.SetDeviceState(1u, texture);
        (void)DrawProjectionQuad(0.0f, 0.0f, size, size,
                                 runtime.DeviceDepth(), reciprocal_w,
                                 0.5f / size, runtime);
        (void)runtime.SetDeviceState(6u, 1u);
        (void)runtime.SetDeviceState(8u, 1u);
        (void)runtime.SetDeviceState(10u, 5u);
        (void)runtime.SetDeviceState(11u, 6u);
        runtime.EndUpdate(camera);
    }
    runtime.WriteWord(camera + 96u, 0u);
    return 1u;
}

std::uint32_t CameraTexturePass::RenderToTextureRepeated(
    const std::uint32_t first_raster,
    const std::uint32_t second_raster,
    const std::uint32_t camera, std::uint32_t iterations,
    ICameraTexturePassRuntime& runtime) {
    const auto size = static_cast<float>(
        runtime.ReadSignedWord(first_raster + 12u));
    const auto uv_offset = 1.0f / size;
    const auto reciprocal_w = 1.0f / runtime.ReadFloat(camera + 132u);
    while (iterations != 0u) {
        runtime.WriteWord(camera + 96u, second_raster);
        if (runtime.BeginUpdate(camera) != 0u) {
            (void)runtime.SetDeviceState(10u, 2u);
            (void)runtime.SetDeviceState(11u, 1u);
            (void)runtime.SetDeviceState(6u, 0u);
            (void)runtime.SetDeviceState(8u, 0u);
            (void)runtime.SetDeviceState(9u, 2u);
            (void)runtime.SetDeviceState(1u, first_raster);
            (void)DrawProjectionQuad(0.0f, 0.0f, size, size,
                                     runtime.DeviceDepth(), reciprocal_w,
                                     uv_offset, runtime);
            runtime.EndUpdate(camera);
        }
        runtime.WriteWord(camera + 96u, first_raster);
        if (runtime.BeginUpdate(camera) != 0u) {
            (void)runtime.SetDeviceState(1u, second_raster);
            (void)DrawProjectionQuad(0.0f, 0.0f, size, size,
                                     runtime.DeviceDepth(), reciprocal_w,
                                     uv_offset * 0.0f, runtime);
            (void)runtime.SetDeviceState(6u, 1u);
            (void)runtime.SetDeviceState(8u, 1u);
            (void)runtime.SetDeviceState(10u, 5u);
            (void)runtime.SetDeviceState(11u, 6u);
            runtime.EndUpdate(camera);
        }
        --iterations;
    }
    runtime.WriteWord(camera + 96u, 0u);
    return 1u;
}

std::uint32_t CameraTexturePass::DrawProjectionQuad(
    const float left, const float top, const float right,
    const float bottom, const float depth, const float reciprocal_w,
    const float uv_offset, ICameraTexturePassRuntime& runtime) {
    const auto upper = uv_offset + 1.0f;
    const std::array<CameraTextureVertex, 4u> vertices{{
        {left, top, depth, reciprocal_w, 0xffffffffu, uv_offset, uv_offset},
        {left, bottom, depth, reciprocal_w, 0xffffffffu, uv_offset, upper},
        {right, top, depth, reciprocal_w, 0xffffffffu, upper, uv_offset},
        {right, bottom, depth, reciprocal_w, 0xffffffffu, upper, upper},
    }};
    runtime.DrawTexturedQuad(4u, vertices, 4u);
    return 1u;
}

}  // namespace pal4
