#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class GameMathTables final {
public:
    // IDA 0x667f60 and 0x667f70.
    void InitializeConstantsWrapper();
    void InitializeConstants();
    // IDA 0x667f90.
    [[nodiscard]] std::uint8_t CalculateSquareRootTable();
    // IDA 0x668030, originally labeled GetFloatExponent.
    [[nodiscard]] double ApproximateSquareRoot(float value) const;
    // IDA 0x6680a0: the x87 result is a floating-point value.
    [[nodiscard]] double NormalizeAngle(float value,
                                        std::uint8_t& folded) const;
    // IDA 0x6681c0 and 0x6681e0; both return through the x87 stack.
    [[nodiscard]] double ApproximateInverseSquareRoot(float value) const;
    [[nodiscard]] double ApproximateCosine(float value) const;
    // IDA 0x668250 and 0x6682e0.
    [[nodiscard]] double ApproximateArcsine(float value) const;
    [[nodiscard]] double ApproximateArccosine(float value) const;
    // IDA 0x668410 and 0x668420 write a distinct global slot.
    void InitializeVectorConstantsWrapper();
    void InitializeVectorConstants();

    float three_half_pi{};
    float vector_three_half_pi{};
    bool square_root_ready{};
    std::array<std::uint32_t, 0x2000> square_root_one{};
    std::array<std::uint32_t, 0x2000> square_root_two{};
};

}  // namespace pal4
