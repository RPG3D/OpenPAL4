#include "pal4/math_sse_approximation.h"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::MathSseVectorFrame frame{};
    const auto input_address = reinterpret_cast<std::uintptr_t>(&frame.input);
    Check((input_address & 15u) == 0u &&
              input_address - reinterpret_cast<std::uintptr_t>(&frame) == 48u,
          "vector input is aligned with 48 writable prefix bytes");
    for (std::size_t offset = 0; offset < 48u; offset += 8u) {
        const std::uint64_t pattern = 0x1122334455667788ull + offset;
        std::memcpy(reinterpret_cast<void*>(input_address - 48u + offset),
                    &pattern, sizeof(pattern));
    }
    Check(frame.input[0u] == 0u && frame.input[3u] == 0u,
          "six MMX scratch writes do not overwrite the input lanes");

    const auto sine_zero = pal4::MathSseApproximation::Sine(0.0f);
    const auto sine_negative_zero = pal4::MathSseApproximation::Sine(-0.0f);
    Check(std::bit_cast<std::uint32_t>(sine_zero) == 0u &&
              std::bit_cast<std::uint32_t>(sine_negative_zero) == 0x80000000u,
          "sine preserves the source sign at zero");
    Check(std::fabs(pal4::MathSseApproximation::Cosine(0.0f) - 1.0f) < 0.001f,
          "cosine starts at one");

    for (const float radians : {-3.0f, -2.0f, -1.0f, -0.5f,
                                0.5f, 1.0f, 2.0f, 3.0f}) {
        const auto sine = pal4::MathSseApproximation::Sine(radians);
        const auto cosine = pal4::MathSseApproximation::Cosine(radians);
        Check(std::fabs(sine - std::sin(radians)) < 0.002f,
              "sine polynomial and quadrant reduction");
        Check(std::fabs(cosine - std::cos(radians)) < 0.002f,
              "cosine polynomial and quadrant reduction");
        Check(std::fabs(sine + pal4::MathSseApproximation::Sine(-radians)) <
                  0.000001f,
              "sine input-sign handling");
        Check(std::bit_cast<std::uint32_t>(cosine) ==
                  std::bit_cast<std::uint32_t>(
                      pal4::MathSseApproximation::Cosine(-radians)),
              "cosine absolute-value handling");
    }
}
