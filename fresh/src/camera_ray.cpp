#include "pal4/camera_ray.h"

namespace pal4 {

std::uint32_t CameraRayAccess::Calculate(
    CameraDataState& state, CameraRay* output,
    const CameraPixel& pixel) {
    const auto camera = state.At(0);
    if (camera == 0u || output == nullptr) return 0u;
    runtime_.CalculatePixelRay(camera, *output, pixel);
    return 1u;
}

std::uint32_t CameraRayAccess::CalculateNormalized(
    CameraDataState& state, CameraRay* output,
    const CameraPixel& pixel) {
    const auto camera = state.At(0);
    if (camera == 0u || output == nullptr) return 0u;
    runtime_.CalculatePixelRay(camera, *output, pixel);
    static_cast<void>(runtime_.NormalizeDirection(*output));
    return 1u;
}

std::uint32_t CameraRayAccess::Intersect(
    CameraDataState& state, std::uint32_t output,
    const CameraPixel& pixel) {
    const auto camera = state.At(0);
    if (camera == 0u) return 0u;
    return runtime_.CalculateIntersection(state, 0x0cu, camera,
                                          output, pixel);
}

}  // namespace pal4
