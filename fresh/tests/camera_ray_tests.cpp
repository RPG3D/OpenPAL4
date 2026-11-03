#include "pal4/camera_ray.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

class Runtime final : public pal4::ICameraRayRuntime {
public:
    std::vector<std::string> calls;

    void CalculatePixelRay(std::uint32_t camera,
                           pal4::CameraRay& ray,
                           const pal4::CameraPixel& pixel) override {
        Check(camera == 0x1000u &&
                  pixel == pal4::CameraPixel{120.25f, 240.75f},
              "pixel-ray camera and coordinate operands");
        ray.origin = {1.0f, 2.0f, 3.0f};
        ray.direction = {3.0f, 4.0f, 0.0f};
        calls.emplace_back("calculate ray");
    }
    float NormalizeDirection(pal4::CameraRay& ray) override {
        Check(ray.origin == std::array<float, 3>{1.0f, 2.0f, 3.0f} &&
                  ray.direction == std::array<float, 3>{3.0f, 4.0f, 0.0f},
              "normalization receives ray direction at +0xc");
        ray.direction = {0.6f, 0.8f, 0.0f};
        calls.emplace_back("normalize");
        return 5.0f;
    }
    std::uint32_t CalculateIntersection(
        pal4::CameraDataState& state, std::uint32_t context_offset,
        std::uint32_t camera, std::uint32_t output,
        const pal4::CameraPixel& pixel) override {
        Check(state.At(0) == 0x1000u && context_offset == 0x0cu &&
                  camera == 0x1000u && output == 0x5000u &&
                  pixel == pal4::CameraPixel{120.25f, 240.75f},
              "pixel intersection context and operand order");
        calls.emplace_back("intersection");
        return 0x77u;
    }
};

void Cases() {
    Runtime runtime;
    pal4::CameraRayAccess access(runtime);
    pal4::CameraDataState state;
    pal4::CameraRay ray;
    const pal4::CameraPixel pixel{120.25f, 240.75f};

    Check(access.Calculate(state, &ray, pixel) == 0u &&
              access.CalculateNormalized(state, &ray, pixel) == 0u &&
              access.Intersect(state, 0x5000u, pixel) == 0u &&
              runtime.calls.empty(),
          "missing camera skips ray and intersection services");
    state.At(0) = 0x1000u;
    Check(access.Calculate(state, nullptr, pixel) == 0u &&
              access.CalculateNormalized(state, nullptr, pixel) == 0u &&
              runtime.calls.empty(),
          "null ray output skips native pixel-ray calculation");

    Check(access.Calculate(state, &ray, pixel) == 1u &&
              ray.origin == std::array<float, 3>{1.0f, 2.0f, 3.0f} &&
              ray.direction == std::array<float, 3>{3.0f, 4.0f, 0.0f} &&
              runtime.calls == std::vector<std::string>{"calculate ray"},
          "basic pixel ray preserves native direction");
    runtime.calls.clear();
    Check(access.CalculateNormalized(state, &ray, pixel) == 1u &&
              ray.direction == std::array<float, 3>{0.6f, 0.8f, 0.0f} &&
              runtime.calls == std::vector<std::string>{
                  "calculate ray", "normalize"},
          "normalized pixel ray normalizes after calculation");

    runtime.calls.clear();
    Check(access.Intersect(state, 0x5000u, pixel) == 0x77u &&
              runtime.calls == std::vector<std::string>{"intersection"},
          "pixel intersection forwards native result");
}

}  // namespace

int main() {
    try {
        Cases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
