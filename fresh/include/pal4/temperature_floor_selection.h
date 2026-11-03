#pragma once
#include <array>
#include <cstdint>
namespace pal4 {
inline constexpr std::array<std::uint32_t,4> kTemperatureFloorRgba{
    0xffff99ccu,0xffff6633u,0xffff00ffu,0xff0000ffu};
// 0x5B4900 game callback, TemperatureManager::SelectFloorColor.
// A missing color leaves the previous selection intact.
inline std::uint32_t SelectTemperatureFloorColor(
    const std::array<std::uint32_t,5>& colors,std::uint32_t pixel,
    std::uint32_t& selected) {
    for(std::uint32_t i=0;i<colors.size();++i)
        if(colors[i]==pixel) {selected=i;return i;}
    return 5;
}
} // namespace pal4
