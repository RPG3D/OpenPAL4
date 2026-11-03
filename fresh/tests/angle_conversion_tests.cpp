#include "pal4/angle_conversion.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IAngleConversionRuntime {
public:
    double AngleFromCosine(float cosine) override {
        argument = cosine;
        ++calls;
        return radians;
    }

    float argument = 0.0f;
    double radians = 1.25;
    unsigned calls = 0;
};

}  // namespace

int main() {
    Runtime runtime;
    constexpr float kScale = std::bit_cast<float>(0x42652ee1u);
    const double expected = runtime.radians * kScale;

    Check(pal4::AngleConversion::ConvertToDegrees(
              {99.0f, -81.0f, 0.5f}, runtime) == expected &&
              runtime.argument == 0.5f && runtime.calls == 1,
          "uses Z component and exact conversion constant");
    Check(pal4::AngleConversion::ConvertToDegrees(
              {-12.0f, 33.0f, -0.5f}, runtime) == 360.0 - expected &&
              runtime.argument == -0.5f && runtime.calls == 2,
          "negative X reverses the degree result");
    Check(pal4::AngleConversion::ConvertToDegrees(
              {-1.0f, 0.0f, 0.5f}, runtime) == 360.0 - expected &&
              runtime.argument == 0.5f,
          "left target with positive Z uses the reflected heading (M06 light1_f)");
    Check(pal4::AngleConversion::ConvertToDegrees(
              {1.0f, 0.0f, -0.5f}, runtime) == expected &&
              runtime.argument == -0.5f,
          "right target with negative Z keeps the arccos heading");
    Check(pal4::AngleConversion::ConvertToDegrees(
              {0.0f, 0.0f, -0.0f}, runtime) == expected &&
              !std::signbit(runtime.argument),
          "original dot adds positive zero terms before negative-zero Z");
    Check(pal4::AngleConversion::ConvertToDegrees(
              {0.0f, 0.0f, std::numeric_limits<float>::quiet_NaN()},
              runtime) == expected && std::isnan(runtime.argument),
          "unordered Z does not reverse result");
    Check(pal4::AngleConversion::ConvertToDegrees(
              {std::numeric_limits<float>::quiet_NaN(), 0.0f, 0.5f},
              runtime) == expected && std::isnan(runtime.argument),
          "original zero products preserve unordered X in the arccos argument");
}
