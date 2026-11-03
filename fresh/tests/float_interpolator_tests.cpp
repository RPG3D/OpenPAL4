#include "pal4/float_interpolator.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

class Runtime final : public pal4::IFloatInterpolationRuntime {
public:
    double CosineApproximation(float radians) override {
        Check(std::bit_cast<std::uint32_t>(radians) == 0x3fc90fdbu,
              "smooth mode passes float-rounded half-pi to cosine helper");
        ++calls;
        return cosine;
    }
    unsigned calls = 0;
    double cosine = 0.0;
};

}  // namespace

int main() {
    Runtime runtime;
    Check(pal4::FloatInterpolator::Interpolate(
              10.f, 30.f, 0.25f, 0, runtime) == 15.0 &&
              runtime.calls == 0u,
          "mode zero performs linear interpolation without cosine call");
    Check(pal4::FloatInterpolator::Interpolate(
              10.f, 30.f, 0.25f, 2, runtime) == 15.0 &&
              runtime.calls == 0u,
          "all modes except one use linear branch");
    Check(pal4::FloatInterpolator::Interpolate(
              10.f, 30.f, 0.5f, 1, runtime) == 20.0 &&
              runtime.calls == 1u,
          "smooth mode uses half-weight when cosine returns zero");
    // Actual M10 camera012 inputs, confirmed by original662947..66295F
    // instruction-prefix oracle at PC24, including equal-endpoint drift.
    const auto phase = std::bit_cast<float>(0x3e9bcd71u);
    Check(std::bit_cast<std::uint32_t>(static_cast<float>(pal4::FloatInterpolator::Interpolate(
              241.39990234375f,298.19989013671875f,phase,0,runtime)))==0x43815796u,
          "linear interpolation preserves original camera scalar PC24 result");
    Check(std::bit_cast<std::uint32_t>(static_cast<float>(pal4::FloatInterpolator::Interpolate(
              26.53019905090332f,26.53019905090332f,phase,0,runtime)))==0x41d43ddau,
          "equal linear endpoints retain original PC24 weighted-sum rounding");
    runtime.cosine = 1.0;
    Check(pal4::FloatInterpolator::Interpolate(
              10.f, 30.f, 0.5f, 1, runtime) == 10.0,
          "cosine one selects start value");
    runtime.cosine = -1.0;
    Check(pal4::FloatInterpolator::Interpolate(
              10.f, 30.f, 0.5f, 1, runtime) == 30.0,
          "cosine negative one selects end value");
}
