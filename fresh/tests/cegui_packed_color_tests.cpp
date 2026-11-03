#include "pal4/cegui_packed_color.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class ConversionRuntime final : public pal4::CeguiPackedColor::IFloatToInt {
public:
    std::vector<double> inputs;
    std::array<std::int32_t, 4> outputs{257, -2, 510, 1025};

    std::int32_t Convert(const double value) override {
        const auto index = inputs.size();
        inputs.push_back(value);
        return outputs.at(index);
    }
};

void SetAllBytesToFFAndReturnSelf() {
    pal4::CeguiPackedColor color;
    Check(&color.SetAllBytesFF() == &color &&
              color.Bytes() == std::array<std::uint8_t, 4>{
                                   0xffu, 0xffu, 0xffu, 0xffu},
          "four-byte initializer writes FF and returns the object");
}

void FloatConversionPreservesObservedOrderAndLowBytes() {
    pal4::CeguiPackedColor color;
    ConversionRuntime runtime;
    Check(color.SetFromFloats(0.75f, 0.25f, 0.5f, 1.0f, runtime) == 1025 &&
              color.Bytes() == std::array<std::uint8_t, 4>{
                                   1u, 254u, 254u, 1u},
          "float conversion writes low bytes and returns full final integer");
    Check(runtime.inputs == std::vector<double>{
              static_cast<double>(0.25f) * 255.0,
              static_cast<double>(0.5f) * 255.0,
              static_cast<double>(1.0f) * 255.0,
              static_cast<double>(0.75f) * 255.0},
          "conversion visits second, third, fourth, then first parameter");
}

void ConversionOverwritesWhiteBytes() {
    pal4::CeguiPackedColor color;
    ConversionRuntime runtime;
    static_cast<void>(color.SetAllBytesFF());
    runtime.outputs = {0, 1, 2, 3};
    Check(color.SetFromFloats(0.0f, 0.0f, 0.0f, 0.0f, runtime) == 3 &&
              color.Bytes() == std::array<std::uint8_t, 4>{0u, 1u, 2u, 3u},
          "all four bytes are overwritten in sequence");
}

void HostFloatToIntSupportsNormalColors() {
    pal4::CeguiPackedColor color;
    pal4::CeguiHostFloatToInt runtime;
    Check(color.SetFromFloats(1.0f, 0.5f, 0.25f, 0.0f,
                              runtime) == 255 &&
              color.Bytes() == std::array<std::uint8_t, 4>{
                                   127u, 63u, 0u, 255u},
          "host adapter truncates ordinary normalized color channels");
    Check(runtime.Convert(std::numeric_limits<double>::infinity()) ==
              std::numeric_limits<std::int32_t>::min() &&
              runtime.Convert(2147483648.0) ==
                  std::numeric_limits<std::int32_t>::min(),
          "host adapter defines invalid and out-of-range conversion result");
}

}  // namespace

int main() {
    SetAllBytesToFFAndReturnSelf();
    FloatConversionPreservesObservedOrderAndLowBytes();
    ConversionOverwritesWhiteBytes();
    HostFloatToIntSupportsNormalColors();
    std::cout << "4 CEGUI packed-color test cases passed\n";
}
