#include "pal4/float_interpolator.h"

#include <bit>
#include <cstdint>

namespace pal4 {

double FloatInterpolator::Interpolate(
    const float start, const float end, const float fraction,
    const int mode, IFloatInterpolationRuntime& runtime) {
    if (mode != 1) {
        // IDA662947..66295D: PC24 rounds the complement, each product,
        // and the sum. Equal endpoints must still follow these operations.
        const float inverse = 1.0f - fraction;
        const float left = inverse * start;
        const float right = end * fraction;
        return static_cast<float>(left + right);
    }

    constexpr auto kPi = std::bit_cast<float>(0x40490fdbu);
    const float radians = static_cast<float>(
        static_cast<double>(fraction) * kPi);
    const double weight =
        (1.0 - runtime.CosineApproximation(radians)) * 0.5;
    return (1.0 - weight) * start + weight * end;
}

}  // namespace pal4
