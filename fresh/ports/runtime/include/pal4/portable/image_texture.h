#pragma once
#include "pal4/portable/scene.h"
#include <span>
namespace pal4::portable {
Texture DecodeOriginalTexture(std::span<const std::uint8_t> bytes,bool dds);
}
