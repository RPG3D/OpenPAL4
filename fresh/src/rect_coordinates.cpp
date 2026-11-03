#include "pal4/rect_coordinates.h"

namespace pal4 {

RectCoordinateWords& RectCoordinates::Initialize(
    const std::uint32_t x, const std::uint32_t y,
    const std::uint32_t width, const std::uint32_t height,
    RectCoordinateWords& output) {
    output = {x, y, x + width, y + height};
    return output;
}

}  // namespace pal4
