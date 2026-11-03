#pragma once

#include "pal4/math_trigonometry.h"

#include <array>
#include <cstddef>

namespace pal4 {

struct alignas(16) MathSseVectorFrame final {
    std::array<std::byte, 48> scratch{};
    MathSimdRegister input{};
};

static_assert(offsetof(MathSseVectorFrame, input) == 48);
static_assert(sizeof(MathSseVectorFrame) == 64);

class MathSseApproximation final {
public:
    // First output lane of IDA 0x7927A4 and 0x792868.
    [[nodiscard]] static float Sine(float radians);
    [[nodiscard]] static float Cosine(float radians);
};

}  // namespace pal4
