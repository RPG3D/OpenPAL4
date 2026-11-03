#include "pal4/camera_task_object.h"

#include <cmath>
#include <bit>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Near(float actual, float expected, float tolerance = 0.02f) {
    return std::fabs(actual - expected) <= tolerance;
}

void CheckAxes() {
    pal4::CameraTaskVector destination{9.0f, 9.0f, 9.0f};
    Check(&pal4::CameraTaskOrientation::ToEuler({0, 0, 0, 1},
                                                destination) ==
                  &destination &&
              Near(destination.x, 0.0f) && Near(destination.y, 0.0f) &&
              Near(destination.z, 0.0f),
          "identity quaternion resets all Euler angles and returns output");

    const auto sine_15 = std::sin(3.14159265358979323846 / 12.0);
    const auto cosine_15 = std::cos(3.14159265358979323846 / 12.0);
    static_cast<void>(pal4::CameraTaskOrientation::ToEuler(
        {static_cast<float>(sine_15), 0.0f, 0.0f,
         static_cast<float>(cosine_15)},
        destination));
    Check(Near(destination.x, 30.0f) && Near(destination.y, 0.0f) &&
              Near(destination.z, 0.0f),
          "general branch converts X rotation from radians to degrees");

    const auto half_root = std::sqrt(0.5f);
    static_cast<void>(pal4::CameraTaskOrientation::ToEuler(
        {0.0f, half_root, 0.0f, half_root}, destination));
    Check(Near(destination.x, 0.0f) && Near(destination.y, 90.0f) &&
              Near(destination.z, 0.0f),
          "Y rotation uses second Euler output");
    static_cast<void>(pal4::CameraTaskOrientation::ToEuler(
        {0.0f, 0.0f, half_root, half_root}, destination));
    Check(Near(destination.x, 0.0f) && Near(destination.y, 0.0f) &&
              Near(destination.z, 90.0f),
          "Z rotation uses third Euler output");
}

void CheckOriginalPitchPrecision() {
    pal4::CameraTaskVector destination;
    // Original5930D0..59310F at PC24 produces sine3EB58E22. Retain the
    // public double-asin boundary and float degree storage: old pitch41A626C5.
    (void)pal4::CameraTaskOrientation::ToEuler({.12f,.33f,-.21f,.9f},destination);
    Check(std::bit_cast<std::uint32_t>(destination.x)==0x41a626c6u,
          "camera quaternion pitch preserves original PC24 sine and double CRT asin boundary");
}

void CheckInjectedMathBoundary() {
    struct Math final : pal4::ICameraTaskOrientationMath {
        int asin_calls{}, atan_calls{};
        float Arcsine(float value) override {
            ++asin_calls;
            Check(std::bit_cast<std::uint32_t>(value)==0x3e77ba94u,
                  "actual captured quaternion supplies original PC24 sine to library");
            return std::bit_cast<float>(0x3e7a3614u);
        }
        float Arctangent2(float y, float x) override {
            ++atan_calls;
            Check(std::isfinite(y) && std::isfinite(x), "finite float operands reach library");
            return atan_calls==1 ? 0.5f : 0.25f;
        }
    } math;
    pal4::CameraTaskVector value;
    (void)pal4::CameraTaskOrientation::ToEuler(
        {std::bit_cast<float>(0x3e8c10c7u),std::bit_cast<float>(0xbdefe8fau),
         std::bit_cast<float>(0x3f74431du),std::bit_cast<float>(0x3d099508u)},value,math);
    Check(math.asin_calls==1 && math.atan_calls==2 &&
          std::bit_cast<std::uint32_t>(value.x)==0x41600032u &&
          value.y==0.5f*57.295780181884766f && value.z==0.25f*57.295780181884766f,
          "injected float radians are converted once into all three Euler outputs");
}

void CheckSingularityAndBoundary() {
    pal4::CameraTaskVector destination;
    static_cast<void>(pal4::CameraTaskOrientation::ToEuler(
        {0.4999f, 0.0f, 0.0f, 1.0f}, destination));
    Check(destination.x > 88.0f && destination.x < 89.5f,
          "below threshold uses asin branch");
    static_cast<void>(pal4::CameraTaskOrientation::ToEuler(
        {0.5f, 0.0f, 0.0f, 1.0f}, destination));
    Check(Near(destination.x, 90.0f) && Near(destination.y, 0.0f) &&
              Near(destination.z, 0.0f),
          "singularity branch clamps third angle to zero");
    static_cast<void>(pal4::CameraTaskOrientation::ToEuler(
        {-0.5f, 0.0f, 0.0f, 1.0f}, destination));
    Check(Near(destination.x, -90.0f),
          "negative singularity preserves rotation sign");

    pal4::CameraTaskGeometryStub stub;
    const auto from_stub = stub.ToEuler({0.5f, 0.0f, 0.0f, 1.0f});
    Check(Near(from_stub.x, 90.0f),
          "camera geometry adapter uses reconstructed Euler conversion");
}

}  // namespace

int main() {
    try {
        CheckAxes();
        CheckOriginalPitchPrecision();
        CheckInjectedMathBoundary();
        CheckSingularityAndBoundary();
        std::cout << "camera_task_orientation_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
