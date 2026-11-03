#pragma once

#include "pal4/camera_data.h"

#include <array>
#include <cstdint>
#include <utility>

namespace pal4 {

class ICameraScreenProjectionRuntime {
public:
    virtual ~ICameraScreenProjectionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ColorRaster(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::pair<std::int32_t, std::int32_t>
    RasterSize(std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::array<float, 3> CalculatePosition(
        std::uint32_t source, std::uint32_t camera,
        std::uint32_t transform_offset) = 0;
    [[nodiscard]] virtual float NearPlane(std::uint32_t camera) = 0;
    [[nodiscard]] virtual float FarPlane(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t CameraType(
        std::uint32_t camera) = 0;
    [[nodiscard]] virtual float ViewWindowX(std::uint32_t camera) = 0;
};

class CameraProjection final {
public:
    explicit CameraProjection(ICameraScreenProjectionRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e9a50: project an object into two screen coordinates.
    std::array<float, 2>& Project(CameraDataState& state,
                                  std::array<float, 2>& output,
                                  std::uint32_t source);
    // 0x5e9b40: convert two screen coordinates and depth to camera space.
    std::array<float, 3>& Unproject(CameraDataState& state,
                                    std::array<float, 3>& output,
                                    const std::array<float, 2>& screen,
                                    float depth);

private:
    ICameraScreenProjectionRuntime& runtime_;
};

}  // namespace pal4
