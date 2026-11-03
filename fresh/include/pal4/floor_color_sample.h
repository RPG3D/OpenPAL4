#pragma once
#include <array>
#include <cstdint>

namespace pal4 {
// Game policy from 0x420BD0. Conversion belongs to the graphics adapter;
// Apply runs before current_alpha changes, as in the original callback path.
template<class Convert, class Apply>
std::uint32_t UpdateFloorColorSample(
    const std::array<std::uint32_t,3>& raw, const std::uint32_t& current_color,
    const std::uint32_t& fallback_alpha, std::uint32_t& current_alpha,
    Convert&& convert, Apply&& apply) {
    std::array<std::uint32_t,3> pixel{};
    for (unsigned i=0;i<3;++i) pixel[i]=convert(raw[i]|0xff000000u);
    auto selected=current_color;
    if (pixel[0]!=selected && pixel[0]==pixel[1]) selected=pixel[0];
    else if (pixel[1]!=selected && pixel[1]==pixel[2]) selected=pixel[1];
    else if (pixel[0]!=selected && pixel[0]==pixel[2]) selected=pixel[0];
    apply(selected);
    const auto a=raw[0]>>24, b=raw[1]>>24, c=raw[2]>>24;
    if (a==b) { current_alpha=a; return a; }
    if (b==c) {
        current_alpha=b;
        // Preserve the original unaligned EAX load, not only its alpha byte.
        return ((raw[1]&0x00ffffffu)<<8)|a;
    }
    if (a==c) { current_alpha=a; return a; }
    current_alpha=fallback_alpha;
    return fallback_alpha;
}
} // namespace pal4
