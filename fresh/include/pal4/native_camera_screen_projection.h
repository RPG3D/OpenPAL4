#pragma once

#include "pal4/camera_projection.h"

namespace pal4 {

// Uses the current SDK camera view matrix. The caller must update/begin the
// camera after changing its frame and pass the active CameraData image.
class NativeCameraScreenProjectionRuntime final
    : public ICameraScreenProjectionRuntime {
public:
    std::uint32_t ColorRaster(std::uint32_t camera) override;
    std::pair<std::int32_t, std::int32_t> RasterSize(std::uint32_t raster) override;
    std::array<float, 3> CalculatePosition(std::uint32_t source,
        std::uint32_t camera, std::uint32_t transform_offset) override;
    float NearPlane(std::uint32_t camera) override;
    float FarPlane(std::uint32_t camera) override;
    std::uint32_t CameraType(std::uint32_t camera) override;
    float ViewWindowX(std::uint32_t camera) override;
};

[[nodiscard]] std::array<float, 2> ProjectNativeCameraScreen(
    CameraDataState& camera, const std::array<float, 3>& position);

}  // namespace pal4
