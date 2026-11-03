#pragma once
#include <bit>
#include <cstdint>

namespace pal4 {
// Game task arithmetic61D23C..61D25A /61D84A..61D86C. Sampling remains
// the caller's public CRT service; preserve each PC24 boundary and addition order.
[[nodiscard]] inline constexpr float RandomizedEffectDuration(
    std::int32_t sample,float initial,float bias) {
    const float width=bias-(-bias);
    const float product=static_cast<float>(sample)*width;
    const float fraction=product*std::bit_cast<float>(0x38000100u);
    const float adjustment=fraction-bias;
    return adjustment+initial;
}
}
