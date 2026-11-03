#pragma once
#include "pal4/portable/scene.h"
#include <span>
namespace pal4::portable {
// 标准DDS头及顶层图像；BC块由第三方bcdec解码，不重写SDK私有解码器。
Texture DecodeDdsTexture(std::span<const std::uint8_t> bytes);
}
