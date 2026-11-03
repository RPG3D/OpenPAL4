#include "pal4/grid_vector_math.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool ok, const char* message) {
    if (!ok) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool Near(const float actual, const float expected) {
    return std::abs(actual - expected) < 0.00001f;
}

}  // namespace

int main() {
    pal4::GridVector3 output{};
    const pal4::GridVector3 velocity{1.0f, 0.0f, 0.0f};
    const pal4::GridVector3 normal{0.0f, 1.0f, 0.0f};
    Check(&pal4::GridVectorMath::ConstrainDirection(
              output, false, velocity, -0.5f, normal) == &output &&
              Near(output[0], std::sqrt(0.75f)) &&
              Near(output[1], -0.5f) && Near(output[2], 0.0f),
          "false flag adjusts when cosine exceeds threshold");
    Check(&pal4::GridVectorMath::ConstrainDirection(
              output, true, velocity, 0.5f, normal) == &output &&
              Near(output[0], std::sqrt(0.75f)) &&
              Near(output[1], 0.5f),
          "true flag adjusts when cosine falls below threshold");
    Check(pal4::GridVectorMath::ConstrainDirection(
              output, false, velocity, 0.5f, normal) == velocity &&
              pal4::GridVectorMath::ConstrainDirection(
                  output, true, velocity, -0.5f, normal) == velocity,
          "opposite threshold branches preserve input");
    const pal4::GridVector3 zero{};
    Check(pal4::GridVectorMath::ConstrainDirection(
              output, false, zero, 0.0f, normal) == zero,
          "zero velocity passes through without division");
    const pal4::GridVector3 parallel{0.0f, 2.0f, 0.0f};
    (void)pal4::GridVectorMath::ConstrainDirection(
        output, false, parallel, 0.5f, normal);
    Check(Near(output[0], 0.0f) && Near(output[1], 1.0f) &&
              Near(output[2], 0.0f),
          "parallel velocity follows native zero-tangent branch");
    std::cout << "Grid vector math tests passed\n";
}
