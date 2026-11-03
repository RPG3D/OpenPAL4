#include "pal4/grid_vector_math.h"
#include "support/random_movement_x87_reference.h"

#include <bit>
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
    // Pair302 helper6 cold forward vector. Original65ED90 normalizes each
    // component before the dot product, giving cosine>1 and preserving it.
    const pal4::GridVector3 captured{0.358367919921875f,0.f,-0.9335803985595703f};
#if defined(_MSC_VER) && defined(_M_IX86)
    Check(pal4::test::MovementVectorLengthPc24(captured)==
              std::bit_cast<float>(0x3f7fffffu),
          "original x87 rounds the captured forward length below one");
    const auto unit=pal4::test::RandomMovementNormalizePc24(captured);
    Check(pal4::test::MovementVectorDotPc24(unit,captured)==
              std::bit_cast<float>(0x3f800001u),
          "original normalized-component dot reaches the preserve branch");
#endif
    (void)pal4::GridVectorMath::ConstrainDirection(output,true,captured,1.f,captured);
    Check(output==captured,
          "threshold1 preserves input instead of adjusting a wide cosine");
    std::cout << "Grid vector math tests passed\n";
    return 0;
}
