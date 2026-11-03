#include "pal4/game_vector_math.h"

#include <cmath>
#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

void NormalizeAndMagnitude() {
    pal4::GameMathTables tables;
    (void)tables.CalculateSquareRootTable();
    pal4::GameVector3 vector{3.0f, 4.0f, 0.0f};
    Check(std::abs(pal4::GameVectorMath::ApproximateMagnitude(
              vector, tables) - 5.0) < 0.001,
          "magnitude uses square-root lookup on length squared");
    pal4::GameVectorMath::Normalize(vector, tables);
    Check(std::abs(vector[0] - 0.6f) < 0.001f &&
          std::abs(vector[1] - 0.8f) < 0.001f && vector[2] == 0.0f,
          "normalization writes all three coordinates");
    pal4::GameVector3 zero{};
    pal4::GameVectorMath::Normalize(zero, tables);
    Check(zero == pal4::GameVector3{},
          "zero vector stays zero after epsilon-scaled normalization");
}

void OriginalPc24Magnitude() {
    pal4::GameMathTables tables;
    (void)tables.CalculateSquareRootTable();
    struct Case {
        std::array<std::uint32_t, 3> input;
        std::uint64_t magnitude;
    };
    // IDA668530..668555 executed with x87 PC24; its resulting float
    // argument is passed to the existing recovered square-root table.
    // Each case distinguishes per-operation rounding from double accumulation.
    constexpr Case cases[] = {
        {{{0x42bc96acu,0x44d7221bu,0x419570beu}},0x409aeecb00000000ull},
        {{{0x447f6105u,0x3f0bdea0u,0x405d017fu}},0x408febf920000000ull},
        {{{0x42011248u,0x42cd9107u,0x42571abau}},0x405e19d280000000ull},
        {{{0x4045c524u,0x3f0a4e33u,0x40183df6u}},0x400f7eba40000000ull},
    };
    for (const auto& sample : cases) {
        pal4::GameVector3 vector;
        for (unsigned i=0; i<3; ++i)
            vector[i]=std::bit_cast<float>(sample.input[i]);
        Check(std::bit_cast<std::uint64_t>(
            pal4::GameVectorMath::ApproximateMagnitude(vector,tables))==sample.magnitude,
            "original PC24 magnitude preserves product and sum rounding");
    }
}

void ClampAfterNormalize() {
    pal4::GameMathTables tables;
    (void)tables.CalculateSquareRootTable();
    pal4::GameVector3 vector{-2.0f, 3.0f, 4.0f};
    pal4::GameVectorMath::NormalizeAndClamp(vector, tables);
    Check(vector[0] >= -1.0f && vector[0] <= 1.0f &&
          vector[1] >= -1.0f && vector[1] <= 1.0f &&
          vector[2] >= -1.0f && vector[2] <= 1.0f &&
          std::abs(vector[0] + 2.0f / std::sqrt(29.0f)) < 0.001f,
          "clamp path normalizes then bounds each component");
}

void AngleBranches() {
    pal4::GameMathTables tables;
    tables.InitializeConstants();
    std::array<float, 11> basis{};
    basis[0] = 0.0f;
    basis[1] = -1.0f;
    basis[2] = 0.0f;
    basis[6] = -1.0f;
    basis[10] = 0.0f;
    pal4::GameVector3 angles{};
    pal4::GameVectorMath::CalculateAngles(basis, angles, tables);
    Check(std::abs(angles[0] - 270.0f) < 0.1f &&
          std::abs(angles[1] - 360.0f) < 0.1f &&
          std::abs(angles[2] - 270.0f) < 0.1f,
          "ordinary path computes and wraps both negative atan2 angles");

    basis[2] = 1.0f;
    basis[4] = 1.0f;
    basis[5] = 0.0f;
    pal4::GameVectorMath::CalculateAngles(basis, angles, tables);
    Check(angles[0] == 0.0f &&
          std::abs(angles[1] - 270.0f) < 0.1f &&
          std::abs(angles[2] - 270.0f) < 0.1f,
          "near-vertical path fixes first angle and uses fallback axes");

    basis[2] = 0.5f;
    basis[0] = 1.0f;
    basis[1] = 0.0f;
    basis[6] = 0.0f;
    basis[10] = 1.0f;
    pal4::GameVectorMath::CalculateAngles(basis, angles, tables);
    Check(std::abs(angles[1] - 330.0f) < 0.2f &&
          std::abs(angles[0]) < 0.1f &&
          std::abs(angles[2]) < 0.1f,
          "positive source Z produces wrapped negative pitch");
}

}  // namespace

int main() {
    NormalizeAndMagnitude();
    OriginalPc24Magnitude();
    ClampAfterNormalize();
    AngleBranches();
}
