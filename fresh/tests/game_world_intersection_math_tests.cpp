#include "pal4/game_world_intersection_math.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    using Math = pal4::GameWorldIntersectionMath;
    const Math::Plane plane{1.f, 2.f, 3.f, 0.f, 1.f, 0.f};
    const Math::Vector point{1.f, 10.f, 3.f};
    Check(Math::CalculateParameter(
              plane, Math::Vector{0.f, 2.f, 0.f}, point) == 8.0,
          "positive dot product yields signed intersection parameter");
    Check(Math::CalculateParameter(
              plane, Math::Vector{0.f, -2.f, 0.f}, point) == 8.0,
          "negative dot product uses the same quotient");
    Check(Math::CalculateParameter(
              plane, Math::Vector{1.f, 0.f, 0.f}, point) ==
              std::numeric_limits<float>::max(),
          "parallel direction returns native float maximum as double");
    Check(Math::CalculateParameter(
              plane, Math::Vector{0.f, 0.0000005f, 0.f}, point) ==
              std::numeric_limits<float>::max(),
          "small positive denominator is rejected");
    Check(Math::CalculateParameter(
              plane, Math::Vector{0.f, -0.0000005f, 0.f}, point) ==
              std::numeric_limits<float>::max(),
          "small negative denominator is rejected");
    const auto nan = std::numeric_limits<float>::quiet_NaN();
    Check(std::isnan(Math::CalculateParameter(
              plane, Math::Vector{0.f, nan, 0.f}, point)),
          "unordered denominator follows the native arithmetic branch");
}
