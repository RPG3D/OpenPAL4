#pragma once
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
namespace pal4 {
// XMLAttribute_ToInt(5F4340)消费CRT signed64 ftol返回值的低32位。
inline std::int32_t XmlFloatToInt(std::uint32_t bits){
    const auto value=static_cast<double>(std::bit_cast<float>(bits));
    const auto integer=(!std::isfinite(value) || value < -9223372036854775808.0 || value >= 9223372036854775808.0)
        ? std::numeric_limits<std::int64_t>::min():static_cast<std::int64_t>(value);
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(integer));
}
}
