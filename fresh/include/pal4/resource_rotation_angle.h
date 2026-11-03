#pragma once
#include <bit>
#include <cmath>
namespace pal4 {
// GameResourceManager 0x66673D/66674C: strictly greater than 360.
// Keep the original reciprocal and float stores; -360 and +360 stay 360.
inline float NormalizeResourceRotationAngle(float value) {
    constexpr float circle=360.0f;
    constexpr auto inverse=std::bit_cast<float>(0x3b360b61u);
    if(value>circle)
        value=static_cast<float>(static_cast<double>(value)-std::trunc(static_cast<double>(value)*inverse)*circle);
    if(value<0)
        value=static_cast<float>(static_cast<double>(value)-std::trunc(static_cast<double>(value)*inverse)*circle+circle);
    return value;
}
}
