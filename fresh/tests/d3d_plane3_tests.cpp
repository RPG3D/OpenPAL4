#include "pal4/d3d_plane3.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void TestHorizontalAndReversedPlanes() {
    pal4::D3dPlane3State plane{};
    pal4::D3dPlane3::Initialize(plane, {0.0f, 3.0f, 0.0f},
                               {0.0f, 3.0f, 1.0f},
                               {1.0f, 3.0f, 0.0f});
    Check(plane.normal.x == 0.0f && plane.normal.y == 1.0f &&
              plane.normal.z == 0.0f && plane.anchor.y == 3.0f &&
              plane.offset == -3.0f &&
              pal4::D3dPlane3::HeightAt(plane, 5.0f, -2.0f) == 3.0,
          "ordered horizontal plane gives positive Y normal and constant height");
    pal4::D3dPlane3::Initialize(plane, {0.0f, 3.0f, 0.0f},
                               {1.0f, 3.0f, 0.0f},
                               {0.0f, 3.0f, 1.0f});
    Check(plane.normal.y == -1.0f && plane.offset == 3.0f &&
              pal4::D3dPlane3::HeightAt(plane, 5.0f, -2.0f) == 3.0,
          "reversed winding reverses normal and offset but preserves height");
}

void TestSlopedAndDegeneratePlanes() {
    pal4::D3dPlane3State plane{};
    pal4::D3dPlane3::Initialize(plane, {0.0f, 3.0f, 0.0f},
                               {1.0f, 4.0f, 0.0f},
                               {0.0f, 5.0f, 1.0f});
    Check(std::fabs(pal4::D3dPlane3::HeightAt(plane, 1.0f, 1.0f) -
                    6.0) < 0.000001,
          "sloped plane height uses X and Z coefficients");
    Check(plane.anchor.x == 0.0f && plane.anchor.y == 3.0f &&
              plane.anchor.z == 0.0f,
          "plane stores first input point");
    plane.normal.y = 0.0f;
    Check(pal4::D3dPlane3::HeightAt(plane, 5.0f, 7.0f) == 0.0,
          "vertical plane height returns zero");
    plane.normal.y = std::numeric_limits<float>::quiet_NaN();
    Check(std::isnan(pal4::D3dPlane3::HeightAt(plane, 5.0f, 7.0f)),
          "unordered normal Y follows division path");

    pal4::D3dPlane3::Initialize(plane, {1.0f, 2.0f, 3.0f},
                               {1.0f, 2.0f, 3.0f},
                               {1.0f, 2.0f, 3.0f});
    Check(plane.normal.x == 0.0f && plane.normal.y == 0.0f &&
              plane.normal.z == 0.0f && plane.offset == 0.0f &&
              pal4::D3dPlane3::HeightAt(plane, 1.0f, 3.0f) == 0.0,
          "degenerate points yield a zero normal and zero height");
}

void TestOriginalM06FloorRounding() {
    // Read-only live208: both games have identical plane/vertex bytes and X/Z.
    // Original height is41554297; a widened expression produces41554295.
    pal4::D3dPlane3State plane{
        {std::bit_cast<float>(0xbc7ff001u),
         std::bit_cast<float>(0x3f7caf94u),
         std::bit_cast<float>(0xbe2374d4u)},
        {603.05224609375f, 0.5999984741210938f, 833.7821044921875f},
        std::bit_cast<float>(0x430debc2u)};
    const auto height = pal4::D3dPlane3::HeightAt(
        plane, 489.20660400390625f, 923.6323852539062f);
    Check(std::bit_cast<std::uint32_t>(static_cast<float>(height)) ==
              0x41554297u,
          "original sloped M06 floor rounds each operation before promotion");
}

}  // namespace

int main() {
    TestHorizontalAndReversedPlanes();
    TestSlopedAndDegeneratePlanes();
    TestOriginalM06FloorRounding();
}
