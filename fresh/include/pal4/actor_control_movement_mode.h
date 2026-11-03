#pragma once
#include <bit>
#include <cstdint>
namespace pal4 {
// 原428190的纯数值部分；保留未知模式的倍率与无符号返回运算。
inline std::uint32_t ApplyActorMovementMode(std::uint32_t mode,float& multiplier,float& speed) {
    switch(mode) {
    case 0:multiplier=std::bit_cast<float>(0x3ecccccdU);break;
    case 1:multiplier=1;break;
    case 2:multiplier=1.5f;break;
    default:break;
    }
    speed=static_cast<float>(static_cast<double>(multiplier)*
        static_cast<double>(std::bit_cast<float>(0x4329e666U)));
    return mode<=2?0:mode-2;
}
}
