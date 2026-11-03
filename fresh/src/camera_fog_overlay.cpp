#include "pal4/camera_fog_overlay.h"

namespace pal4 {
namespace {

std::uint32_t ReplicateGray(const std::uint32_t value) {
    auto color = value;
    color = (color << 8u) | value;
    color = (color << 8u) | value;
    return (color << 8u) | value;
}

}  // namespace

std::uint32_t CameraFogOverlay::Render(
    const std::uint32_t camera, const std::uint32_t raster,
    const float amount, const float distance, const float scale,
    const bool enhanced, ICameraFogRuntime& runtime) {
    const auto magnitude = amount < 0.0f ? -amount : amount;
    const auto base = static_cast<std::uint32_t>(
        runtime.RoundToInt((1.0f - magnitude) * 255.0f));
    auto top = base;
    auto bottom = base;
    if (enhanced) {
        const auto raw_extra = runtime.Ftol(
            255.0 - static_cast<double>(distance) / scale * 96.0);
        const auto extra = raw_extra < 0 ? 0u :
            static_cast<std::uint32_t>(raw_extra);
        bottom = base + extra;
        if (bottom > 255u) bottom = 255u;
        top = extra + ((bottom + 255u) >> 1u);
        if (top > 255u) top = 255u;
    }

    runtime.WriteWord(camera + 96u, raster);
    if (runtime.BeginUpdate(camera) != 0u) {
        const auto bound_raster = runtime.ReadWord(camera + 96u);
        const auto width = static_cast<float>(
            runtime.ReadSignedWord(bound_raster + 12u));
        const auto height = static_cast<float>(
            runtime.ReadSignedWord(bound_raster + 16u));
        const auto reciprocal_w = 1.0f / runtime.ReadFloat(camera + 128u);
        const auto top_color = ReplicateGray(top);
        const auto bottom_color = ReplicateGray(bottom);
        const std::array<CameraScreenVertex, 4u> vertices{{
            {0.0f, 0.0f, runtime.DeviceDepth(), reciprocal_w, top_color},
            {0.0f, height, runtime.DeviceDepth(), reciprocal_w, bottom_color},
            {width, 0.0f, runtime.DeviceDepth(), reciprocal_w, top_color},
            {width, height, runtime.DeviceDepth(), reciprocal_w, bottom_color},
        }};
        (void)runtime.SetDeviceState(12u, 1u);
        (void)runtime.SetDeviceState(1u, 0u);
        (void)runtime.SetDeviceState(6u, 0u);
        (void)runtime.SetDeviceState(8u, 0u);
        (void)runtime.SetDeviceState(29u, 8u);
        (void)runtime.SetDeviceState(10u, 10u);
        (void)runtime.SetDeviceState(11u, 2u);
        runtime.DrawQuad(4u, vertices, 4u);
        (void)runtime.SetDeviceState(6u, 1u);
        (void)runtime.SetDeviceState(8u, 1u);
        (void)runtime.SetDeviceState(29u, 5u);
        (void)runtime.SetDeviceState(10u, 5u);
        (void)runtime.SetDeviceState(11u, 6u);
        runtime.EndUpdate(camera);
    }
    runtime.WriteWord(camera + 96u, 0u);
    return 1u;
}

}  // namespace pal4
