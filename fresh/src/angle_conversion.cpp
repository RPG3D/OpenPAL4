#include "pal4/angle_conversion.h"

#include <bit>
#include <cstdint>

namespace pal4 {

double AngleConversion::ConvertToDegrees(
    const std::array<float, 3>& direction,
    IAngleConversionRuntime& runtime) {
    constexpr float kRadiansToDegrees = std::bit_cast<float>(0x42652ee1u);
    // IDA6629A0 dots against (0,0,1), then uses a different cyclic
    // dot product for the hemisphere. Keep zero products for NaN/Inf behavior.
    const float cosine = static_cast<float>(
        (static_cast<double>(direction[1]) * 0.0 +
         static_cast<double>(direction[0]) * 0.0) + direction[2]);
    const double degrees = runtime.AngleFromCosine(cosine) * kRadiansToDegrees;
    const double hemisphere =
        (static_cast<double>(direction[1]) * 0.0 +
         static_cast<double>(direction[2]) * 0.0) + direction[0];
    return hemisphere < 0.0 ? 360.0 - degrees : degrees;
}

}  // namespace pal4
