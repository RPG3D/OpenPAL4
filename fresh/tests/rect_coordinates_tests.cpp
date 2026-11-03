#include "pal4/rect_coordinates.h"

#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::RectCoordinateWords result{99u, 99u, 99u, 99u};
    Check(&pal4::RectCoordinates::Initialize(10u, 20u, 30u, 40u, result) ==
              &result &&
              result == pal4::RectCoordinateWords{10u, 20u, 40u, 60u},
          "rectangle stores origin and extent-derived endpoints");
    Check(&pal4::RectCoordinates::Initialize(
              0xfffffffeu, 0xffffffffu, 4u, 2u, result) == &result &&
              result == pal4::RectCoordinateWords{
                  0xfffffffeu, 0xffffffffu, 2u, 1u},
          "endpoint arithmetic retains native 32-bit wraparound");
}
