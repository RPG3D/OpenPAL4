#include "pal4/math_runtime_square_root_table.h"

#include <bit>
#include <cmath>

namespace pal4 {

namespace {

class HostSquareRootEvaluator final : public IMathSquareRootEvaluator {
public:
    double SquareRoot(const double value) override { return std::sqrt(value); }
};

}  // namespace

void MathRuntimeSquareRootTable::Initialize() {
    HostSquareRootEvaluator evaluator;
    Initialize(evaluator);
}

void MathRuntimeSquareRootTable::Initialize(
    IMathSquareRootEvaluator& evaluator) {
    for (std::uint32_t index = 0; index < kEntriesPerHalf; ++index) {
        const auto even_high = (0x7fe000u | index) << 7;
        const auto odd_high = (0x800000u | index) << 7;
        const auto even_input = std::bit_cast<double>(
            static_cast<std::uint64_t>(even_high) << 32);
        const auto odd_input = std::bit_cast<double>(
            static_cast<std::uint64_t>(odd_high) << 32);
        even_[index] = static_cast<std::uint32_t>(
            std::bit_cast<std::uint64_t>(evaluator.SquareRoot(even_input)) >> 32) &
            0xfffffu;
        odd_[index] = static_cast<std::uint32_t>(
            std::bit_cast<std::uint64_t>(evaluator.SquareRoot(odd_input)) >> 32) &
            0xfffffu;
    }
}

double MathRuntimeSquareRootTable::Approximate(const double value) const {
    if (value == 0.0) return 0.0;

    const auto input_bits = std::bit_cast<std::uint64_t>(value);
    const auto input_high = static_cast<std::uint32_t>(input_bits >> 32);
    auto mantissa = input_high & 0xfffffu;
    const auto exponent = (input_high >> 20) - 0x3ffu;
    if ((exponent & 1u) != 0u) mantissa |= 0x100000u;
    const auto index = mantissa >> 7;
    const auto fraction = index < kEntriesPerHalf
                              ? even_[index]
                              : odd_[index - kEntriesPerHalf];
    const auto output_high = ((exponent >> 1) + 0x3ffu) << 20 | fraction;
    const auto output_bits =
        static_cast<std::uint64_t>(output_high) << 32 |
        (input_bits & 0xffffffffu);
    return std::bit_cast<double>(output_bits);
}

}  // namespace pal4
