#pragma once

#include "pal4/camera_data.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct CameraRay {
    std::array<float, 3> origin{};
    std::array<float, 3> direction{};
};
static_assert(sizeof(CameraRay) == 0x18u);

// Original SDK RwV2d pixels: preserve fractional coordinates and raw float words.
using CameraPixel = std::array<float, 2>;

class ICameraRayRuntime {
public:
    virtual ~ICameraRayRuntime() = default;
    virtual void CalculatePixelRay(std::uint32_t camera,
                                   CameraRay& ray,
                                   const CameraPixel& pixel) = 0;
    [[nodiscard]] virtual float NormalizeDirection(CameraRay& ray) = 0;
    [[nodiscard]] virtual std::uint32_t CalculateIntersection(
        CameraDataState& state, std::uint32_t context_offset,
        std::uint32_t camera, std::uint32_t output,
        const CameraPixel& pixel) = 0;
};

class CameraRayAccess final {
public:
    explicit CameraRayAccess(ICameraRayRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5ea100: fill a ray for a pixel.
    [[nodiscard]] std::uint32_t Calculate(
        CameraDataState& state, CameraRay* output,
        const CameraPixel& pixel);
    // 0x5ea130: calculate and normalize the direction at ray +0xc.
    [[nodiscard]] std::uint32_t CalculateNormalized(
        CameraDataState& state, CameraRay* output,
        const CameraPixel& pixel);
    // 0x5ea170: forward camera intersection with pixel and output handle.
    [[nodiscard]] std::uint32_t Intersect(
        CameraDataState& state, std::uint32_t output,
        const CameraPixel& pixel);

private:
    ICameraRayRuntime& runtime_;
};

}  // namespace pal4
