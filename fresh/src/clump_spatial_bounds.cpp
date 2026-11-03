#include "pal4/clump_spatial_bounds.h"

#include <cmath>

namespace pal4 {
namespace {

double ClampCoordinate(const float projected, const float half_extent,
                       double& squared_distance) {
    const double value = projected;
    const double extent = half_extent;
    if (std::fabs(std::fabs(value) - std::fabs(extent)) <
        static_cast<double>(0.0049999999f))
        return value;
    if (value < -extent) {
        const double gap = value + extent;
        squared_distance += gap * gap;
        return -extent;
    }
    if (value > extent) {
        const double gap = value - extent;
        squared_distance += gap * gap;
        return extent;
    }
    return value;
}

std::uint32_t RepackColor(const std::uint32_t color) {
    return (color & 0xff000000u) | ((color & 0xffu) << 16u) |
           (color & 0x0000ff00u) | ((color >> 16u) & 0xffu);
}

}  // namespace

std::uint32_t ClumpSpatialBounds::CalculateBoundingBox(
    ClumpBoundsState& state, const std::uint32_t vertices,
    const std::uint32_t count, const std::uint32_t matrix,
    const std::uint32_t setup, IClumpSpatialBoundsRuntime& runtime) {
    std::array<float, 16> local_matrix{};
    runtime.SetupMatrix(local_matrix, setup);
    runtime.MultiplyMatrix(state.matrix, matrix, local_matrix);
    const auto points = runtime.AllocatePoints(12u * count, 0u);
    try {
        runtime.TransformPoints(points, vertices, count, state.matrix);
        runtime.CalculateBox(state.maximum, state.minimum, points, count);
        state.center = {
            (state.maximum.x + state.minimum.x) * 0.5f,
            (state.maximum.y + state.minimum.y) * 0.5f,
            (state.maximum.z + state.minimum.z) * 0.5f};
        state.half_extent = {
            (state.maximum.x - state.minimum.x) * 0.5f,
            (state.maximum.y - state.minimum.y) * 0.5f,
            (state.maximum.z - state.minimum.z) * 0.5f};
        runtime.TransformVector(state.transformed_center, state.center,
                                state.matrix);
    } catch (...) {
        (void)runtime.FreePoints(points);
        throw;
    }
    return runtime.FreePoints(points);
}

double ClumpSpatialBounds::GetIntersectionDistance(
    const ClumpBoundsState& state, const ClumpBoundsVector& point,
    ClumpBoundsVector* closest) {
    const double dx = static_cast<double>(point.x) -
                      state.transformed_center.x;
    const double dy = static_cast<double>(point.y) -
                      state.transformed_center.y;
    const double dz = static_cast<double>(point.z) -
                      state.transformed_center.z;
    const auto& m = state.matrix;
    const float px = static_cast<float>(m[0] * dx + m[1] * dy + m[2] * dz);
    const float py = static_cast<float>(m[4] * dx + m[5] * dy + m[6] * dz);
    const float pz = static_cast<float>(m[8] * dx + m[9] * dy + m[10] * dz);

    double distance = 0.0;
    const double x = ClampCoordinate(px, state.half_extent.x, distance);
    const double y = ClampCoordinate(py, state.half_extent.y, distance);
    const double z = ClampCoordinate(pz, state.half_extent.z, distance);
    if (closest != nullptr) {
        closest->x = static_cast<float>(
            state.transformed_center.x + x * m[0] + y * m[4] + z * m[8]);
        closest->y = static_cast<float>(
            state.transformed_center.y + x * m[1] + y * m[5] + z * m[9]);
        closest->z = static_cast<float>(
            state.transformed_center.z + x * m[2] + y * m[6] + z * m[10]);
    }
    return distance;
}

bool ClumpSpatialBounds::CheckIntersection(
    const ClumpBoundsState& state, const std::uint32_t camera,
    ClumpBoundsVector* closest, IClumpSpatialBoundsRuntime& runtime) {
    ClumpBoundsVector point;
    runtime.CalculateCameraPosition(point, camera, state.matrix);
    runtime.TransformVector(point, point, state.matrix);
    return GetIntersectionDistance(state, point, closest) <= 0.0;
}

int ClumpSpatialBounds::RenderBoundingBox(
    const ClumpBoundsState& state, const std::uint32_t color,
    const std::uint32_t matrix, IClumpBoundsRenderRuntime& runtime) {
    std::array<ClumpBoundsVertex, 8> vertices{};
    const auto packed = RepackColor(color);
    for (std::uint32_t index = 0; index < vertices.size(); ++index) {
        vertices[index].position = {
            (index & 1u) != 0u ? state.maximum.x : state.minimum.x,
            (index & 2u) != 0u ? state.maximum.y : state.minimum.y,
            (index & 4u) != 0u ? state.maximum.z : state.minimum.z};
        vertices[index].color = packed;
    }
    (void)runtime.SetRenderState(7, 1);
    (void)runtime.SetRenderState(1, 0);
    if (runtime.TransformImmediate(vertices, matrix, 26)) {
        runtime.RenderIndexedLines(1, 24);
        runtime.EndImmediate();
    }
    return runtime.SetRenderState(7, 2);
}

}  // namespace pal4
