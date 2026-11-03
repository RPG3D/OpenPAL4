#include "pal4/cegui_packed_color.h"

#include <cmath>
#include <limits>

namespace pal4 {

CeguiPackedColor& CeguiPackedColor::SetAllBytesFF() noexcept {
    bytes_.fill(0xffu);
    return *this;
}

std::int32_t CeguiPackedColor::SetFromFloats(
    const float first, const float second, const float third,
    const float fourth, IFloatToInt& runtime) {
    const auto scaled_second = runtime.Convert(
        static_cast<double>(second) * 255.0);
    bytes_[0] = static_cast<std::uint8_t>(scaled_second);
    const auto scaled_third = runtime.Convert(
        static_cast<double>(third) * 255.0);
    bytes_[1] = static_cast<std::uint8_t>(scaled_third);
    const auto scaled_fourth = runtime.Convert(
        static_cast<double>(fourth) * 255.0);
    bytes_[2] = static_cast<std::uint8_t>(scaled_fourth);
    const auto scaled_first = runtime.Convert(
        static_cast<double>(first) * 255.0);
    bytes_[3] = static_cast<std::uint8_t>(scaled_first);
    return scaled_first;
}

std::int32_t CeguiHostFloatToInt::Convert(const double value) {
    constexpr auto minimum =
        static_cast<double>(std::numeric_limits<std::int32_t>::min());
    constexpr auto maximum_exclusive =
        static_cast<double>(std::numeric_limits<std::int32_t>::max()) + 1.0;
    if (!std::isfinite(value) || value < minimum ||
        value >= maximum_exclusive) {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(std::trunc(value));
}

}  // namespace pal4
