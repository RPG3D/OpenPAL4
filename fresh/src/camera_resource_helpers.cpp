#include "pal4/camera_resource_helpers.h"

namespace pal4 {

std::uint32_t CameraResourceHelpers::Destroy(
    const std::uint32_t camera, ICameraResourceRuntime& runtime) {
    if (camera == 0u) return 0u;
    const auto frame = runtime.ReadWord(camera + 4u);
    if (frame != 0u) {
        runtime.AttachFrame(camera, 0u);
        runtime.DestroyFrame(frame);
    }
    const auto owned_raster = runtime.ReadWord(camera + 100u);
    if (owned_raster != 0u) {
        runtime.WriteWord(camera + 100u, 0u);
        runtime.DestroyRaster(owned_raster);
    }
    if (runtime.ReadWord(camera + 96u) != 0u)
        runtime.WriteWord(camera + 96u, 0u);
    return runtime.DestroyCamera(camera);
}

std::uint32_t CameraResourceHelpers::Create(
    const std::uint32_t raster_size, ICameraResourceRuntime& runtime) {
    const auto camera = runtime.CreateCamera();
    if (camera != 0u) {
        const auto frame = runtime.CreateFrame();
        runtime.AttachFrame(camera, frame);
        if (runtime.ReadWord(camera + 4u) != 0u) {
            const auto raster = runtime.CreateRaster(
                raster_size, raster_size, 0u, 1u);
            if (raster != 0u) {
                runtime.WriteWord(camera + 100u, raster);
                runtime.SetProjection(camera, 2u);
                return camera;
            }
        }
    }
    (void)Destroy(camera, runtime);
    return 0u;
}

std::uint32_t CameraResourceHelpers::SetClipAndWindow(
    const std::uint32_t camera, const float size,
    ICameraResourceRuntime& runtime) {
    runtime.SetFarClip(camera, size * 1.5f);
    runtime.SetNearClip(camera, size * 0.001f);
    runtime.SetViewWindow(camera, size, size);
    return camera;
}

std::uint32_t CameraResourceHelpers::CopyFrame(
    const std::uint32_t camera, const std::uint32_t source,
    ICameraResourceRuntime& runtime) {
    const auto destination_frame = runtime.ReadWord(camera + 4u);
    const auto source_frame = runtime.ReadWord(source + 4u);
    runtime.CopyMatrix(destination_frame + 16u, source_frame + 16u);
    runtime.UpdateFrame(destination_frame);
    return camera;
}

std::uint32_t CameraResourceHelpers::UpdateTarget(
    const std::uint32_t camera, const std::uint32_t target,
    ICameraResourceRuntime& runtime) {
    const auto frame = runtime.ReadWord(camera + 4u);
    const auto distance = runtime.ReadFloat(camera + 132u) * -0.5f;
    const CameraRenderVector3 offset{
        runtime.ReadFloat(target) - runtime.ReadFloat(frame + 64u) +
            runtime.ReadFloat(frame + 48u) * distance,
        runtime.ReadFloat(target + 4u) - runtime.ReadFloat(frame + 68u) +
            runtime.ReadFloat(frame + 52u) * distance,
        runtime.ReadFloat(target + 8u) - runtime.ReadFloat(frame + 72u) +
            runtime.ReadFloat(frame + 56u) * distance};
    runtime.TranslateFrame(frame, offset, 2u);
    return camera;
}

}  // namespace pal4
