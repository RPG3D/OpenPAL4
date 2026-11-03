#pragma once
#include <bit>
#include <cstdint>
namespace pal4 {
// Game RGBA boundaries61CFF5/61D1A6/61415B/61475B share float84258C.
// Preserve its bits: truncated double0.0039215689 makes255 exceed1 by one ULP.
[[nodiscard]] inline constexpr float NormalizeEffectColorByte(std::uint8_t channel) {
    return static_cast<float>(channel)*std::bit_cast<float>(0x3b808081u);
}
}
