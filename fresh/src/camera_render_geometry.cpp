#include "pal4/camera_render_geometry.h"

#include <cmath>

namespace pal4 {
namespace {

std::uint32_t ComparisonStatus(const float value) {
    if (std::isnan(value)) return 0x4100u;
    if (value < 0.0f) return 0x0100u;
    return value == 0.0f ? 0x4000u : 0u;
}

}  // namespace

std::uint32_t CameraRenderGeometry::UpdateFrustum(
    CameraRenderState& state, const bool special_mode, const float scale,
    ICameraRenderGeometryRuntime& runtime) {
    state.frustum_type = 4u;
    CameraRenderVector3 center{
        runtime.ReadFloat(state.target),
        runtime.ReadFloat(state.target + 4u),
        runtime.ReadFloat(state.target + 8u)};
    const auto radius = runtime.ReadFloat(state.target + 12u);
    const float values[3]{center.x, center.y, center.z};
    for (std::size_t index = 0u; index < 3u; ++index) {
        state.maximum[index] = special_mode ? values[index]
                                             : values[index] + radius;
        state.minimum[index] = special_mode ? values[index]
                                             : values[index] - radius;
    }
    const auto matrix = runtime.ReadWord(state.camera + 4u);
    std::uint32_t status = 0u;
    for (std::uint32_t index = 0u; index < 3u; ++index) {
        const auto component =
            runtime.ReadFloat(matrix + 48u + index * 4u) * scale;
        status = ComparisonStatus(component);
        if (status != 0u)
            state.minimum[index] += component;
        else
            state.maximum[index] += component;
    }
    return status;
}

std::uint32_t CameraRenderGeometry::UpdatePosition(
    CameraRenderState& state, const bool move_along_target,
    ICameraRenderGeometryRuntime& runtime) {
    const auto source_frame = runtime.ReadWord(state.source_frame + 4u);
    const auto camera_frame = runtime.ReadWord(state.camera + 4u);
    const auto camera_matrix = runtime.GetTransform(camera_frame);
    const auto source_matrix = runtime.GetTransform(source_frame);
    runtime.CopyMatrix(camera_matrix, source_matrix);
    runtime.UpdateFrame(camera_frame);
    if (move_along_target) {
        const auto source_position = runtime.GetTransform(source_frame) + 48u;
        CameraRenderVector3 direction{
            runtime.ReadFloat(source_position) - runtime.ReadFloat(state.target),
            runtime.ReadFloat(source_position + 4u) -
                runtime.ReadFloat(state.target + 4u),
            runtime.ReadFloat(source_position + 8u) -
                runtime.ReadFloat(state.target + 8u)};
        runtime.Normalize(direction);
        const auto distance = runtime.ReadFloat(state.dimensions + 12u);
        runtime.WriteFloat(camera_matrix + 48u,
            runtime.ReadFloat(camera_matrix + 48u) + direction.x * distance);
        runtime.WriteFloat(camera_matrix + 52u,
            runtime.ReadFloat(camera_matrix + 52u) + direction.y * distance);
        runtime.WriteFloat(camera_matrix + 56u,
            runtime.ReadFloat(camera_matrix + 56u) + direction.z * distance);
        runtime.ApplyTransform(camera_frame, camera_matrix, 0u);
    }
    return runtime.SetClip(state.camera,
        runtime.ReadFloat(state.dimensions + 12u) * 1.5f);
}

}  // namespace pal4
