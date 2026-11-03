#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

using RectCoordinateWords = std::array<std::uint32_t, 4>;

class RectCoordinates final {
public:
    static RectCoordinateWords& Initialize(
        std::uint32_t x, std::uint32_t y,
        std::uint32_t width, std::uint32_t height,
        RectCoordinateWords& output);
};

}  // namespace pal4
