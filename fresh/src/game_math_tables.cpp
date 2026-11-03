#include "pal4/game_math_tables.h"

#include <bit>
#include <cmath>

namespace pal4 {

namespace {

float ArccosinePolynomial(const float absolute_value) {
    constexpr auto c0 = std::bit_cast<float>(0x3fc90da4u);
    constexpr auto c1 = std::bit_cast<float>(0x3e593484u);
    constexpr auto c2 = std::bit_cast<float>(0x3d981627u);
    constexpr auto c3 = std::bit_cast<float>(0xbc996e30u);
    // IDA66828B..6682B3 / 66831B..668343 execute each x87 operation
    // under PC24. Keep the library sqrt, with the same rounding boundaries
    // on both its input/output and every Horner multiply/add.
    const float root_input = 1.0f - absolute_value;
    const float root = static_cast<float>(std::sqrt(static_cast<double>(root_input)));
    const float first_product = c3 * absolute_value;
    const float first_sum = first_product + c2;
    const float second_product = first_sum * absolute_value;
    const float second_sum = second_product - c1;
    const float third_product = second_sum * absolute_value;
    const float polynomial = third_product + c0;
    return polynomial * root;
}

}  // namespace

void GameMathTables::InitializeConstantsWrapper() {
    InitializeConstants();
}

void GameMathTables::InitializeConstants() {
    three_half_pi = 1.5707964f + 3.1415927f;
}

std::uint8_t GameMathTables::CalculateSquareRootTable() {
    if (square_root_ready) return 1;
    for (std::uint32_t index = 0; index < square_root_one.size(); ++index) {
        const auto first_high = ((0x7fe000u | index) << 7);
        const auto second_high = ((0x800000u | index) << 7);
        const auto first_input = std::bit_cast<double>(
            static_cast<std::uint64_t>(first_high) << 32);
        const auto second_input = std::bit_cast<double>(
            static_cast<std::uint64_t>(second_high) << 32);
        // IDA667FCD/667FF8 FSQRT executes under the game's x87 PC24 mode.
        // Keep the public library sqrt and preserve its single-precision
        // rounding boundary before extracting a double's high word.
        const auto first_root = std::bit_cast<std::uint64_t>(
            static_cast<double>(static_cast<float>(std::sqrt(first_input))));
        const auto second_root = std::bit_cast<std::uint64_t>(
            static_cast<double>(static_cast<float>(std::sqrt(second_input))));
        square_root_one[index] =
            static_cast<std::uint32_t>(first_root >> 32) & 0xfffffu;
        square_root_two[index] =
            static_cast<std::uint32_t>(second_root >> 32) & 0xfffffu;
    }
    square_root_ready = true;
    return 0;
}

double GameMathTables::ApproximateSquareRoot(const float value) const {
    if (value == 0.0f) return 0.0;

    const auto source_bits = std::bit_cast<std::uint64_t>(
        static_cast<double>(value));
    const auto source_high = static_cast<std::uint32_t>(source_bits >> 32);
    auto mantissa = source_high & 0xfffffu;
    const auto exponent = (source_high >> 20) - 0x3ffu;
    if ((exponent & 1u) != 0) mantissa |= 0x100000u;
    const auto table_index = mantissa >> 7;
    const auto fraction = table_index < square_root_one.size()
                              ? square_root_one[table_index]
                              : square_root_two[
                                    table_index - square_root_one.size()];
    const auto output_high = ((exponent >> 1) + 0x3ffu) << 20 | fraction;
    const auto output_bits =
        (static_cast<std::uint64_t>(output_high) << 32) |
        (source_bits & 0xffffffffu);
    return std::bit_cast<double>(output_bits);
}

double GameMathTables::NormalizeAngle(const float value,
                                      std::uint8_t& folded) const {
    constexpr float half_pi = 1.5707964f;
    constexpr float pi = 3.1415927f;
    constexpr float two_pi = 6.2831855f;
    double angle = value;
    while (angle >= two_pi) angle -= two_pi;

    if (angle >= three_half_pi) {
        angle -= three_half_pi;
        return static_cast<double>(half_pi) - angle;
    }
    if (angle >= pi) {
        folded = 1;
        return angle - pi;
    }
    if (angle >= half_pi) {
        folded = 1;
        angle -= half_pi;
        return static_cast<double>(half_pi) - angle;
    }

    while (angle < -static_cast<double>(two_pi)) angle += two_pi;
    if (angle < -static_cast<double>(three_half_pi))
        return angle + three_half_pi + half_pi;
    if (angle < -static_cast<double>(pi)) {
        folded = 1;
        return angle + pi;
    }
    if (angle < -static_cast<double>(half_pi)) {
        folded = 1;
        return angle + half_pi + half_pi;
    }
    return angle;
}

double GameMathTables::ApproximateInverseSquareRoot(
    const float value) const {
    constexpr auto epsilon = std::bit_cast<float>(0x34000000u);
    // IDA6681CA FADD and6681D3 FDIVR also execute under PC24.
    const float denominator = static_cast<float>(ApproximateSquareRoot(value) + epsilon);
    return static_cast<float>(1.0f / denominator);
}

double GameMathTables::ApproximateCosine(const float value) const {
    constexpr auto half_pi = std::bit_cast<float>(0x3fc90fdbu);
    constexpr auto quadratic = std::bit_cast<float>(0x3efe4f76u);
    constexpr auto quartic = std::bit_cast<float>(0x3d17c1beu);
    std::uint8_t folded = 0;
    const auto angle = value >= 0.0f && value < half_pi
                           ? static_cast<double>(value)
                           : NormalizeAngle(value, folded);
    const auto square = angle * angle;
    const auto result = 1.0 + square * (quartic * square - quadratic);
    return folded != 0 ? -result : result;
}

double GameMathTables::ApproximateArcsine(const float value) const {
    if (value > 1.0f || value < -1.0f) return 0.0;
    constexpr auto half_pi = std::bit_cast<float>(0x3fc90fdbu);
    const float result = half_pi - ArccosinePolynomial(std::abs(value));
    return value < 0.0f ? -result : result;
}

double GameMathTables::ApproximateArccosine(const float value) const {
    if (value > 1.0f || value < -1.0f) return 0.0;
    constexpr auto pi = std::bit_cast<float>(0x40490fdbu);
    const auto result =
        ArccosinePolynomial(std::abs(value));
    return value < 0.0f ? static_cast<float>(pi - result) : result;
}

void GameMathTables::InitializeVectorConstantsWrapper() {
    InitializeVectorConstants();
}

void GameMathTables::InitializeVectorConstants() {
    vector_three_half_pi = 1.5707964f + 3.1415927f;
}

}  // namespace pal4
