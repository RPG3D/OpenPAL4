#include "pal4/camera_projection.h"

#include <limits>

namespace pal4 {

std::array<float, 2>& CameraProjection::Project(
    CameraDataState& state, std::array<float, 2>& output,
    std::uint32_t source) {
    constexpr auto missing = std::numeric_limits<float>::max();
    output = {missing, missing};
    const auto camera = state.At(0);
    if (camera == 0u) return output;
    const auto raster = runtime_.ColorRaster(camera);
    if (raster == 0u) return output;

    const auto position = runtime_.CalculatePosition(source, camera, 0x20u);
    const auto depth = position[2];
    if (runtime_.NearPlane(camera) > depth) return output;
    if (runtime_.FarPlane(camera) < depth) return output;

    if (runtime_.CameraType(camera) != 1u) {
        output = {position[0], position[1]};
        return output;
    }
    const auto inverse_view = 1.0 / (2.0 * runtime_.ViewWindowX(camera));
    const auto inverse_depth = 1.0 / static_cast<double>(depth);
    const auto [width, height] = runtime_.RasterSize(raster);
    output[0] = static_cast<float>(
        static_cast<double>(position[0]) * inverse_depth *
        inverse_view * width);
    output[1] = static_cast<float>(
        static_cast<double>(position[1]) * inverse_depth *
        inverse_view * height);
    return output;
}

std::array<float, 3>& CameraProjection::Unproject(
    CameraDataState& state, std::array<float, 3>& output,
    const std::array<float, 2>& screen, float depth) {
    constexpr auto missing = std::numeric_limits<float>::max();
    output = {missing, missing, missing};
    const auto camera = state.At(0);
    if (camera == 0u) return output;
    const auto raster = runtime_.ColorRaster(camera);
    if (raster == 0u) return output;

    const auto [width, height] = runtime_.RasterSize(raster);
    const auto x = static_cast<double>(width) * 0.5 - screen[0];
    const auto y = static_cast<double>(height) * 0.5 - screen[1];
    if (runtime_.CameraType(camera) != 1u) {
        output = {static_cast<float>(x), static_cast<float>(y), depth};
        return output;
    }
    const auto double_view = static_cast<double>(
        runtime_.ViewWindowX(camera)) * 2.0;
    output = {
        static_cast<float>(x * depth * double_view / width),
        static_cast<float>(y * depth * double_view / height),
        depth};
    return output;
}

}  // namespace pal4
