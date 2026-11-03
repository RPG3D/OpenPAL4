#include "pal4/math_runtime_square_root_table.h"
#include "pal4/game_math_tables.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

struct RecordingEvaluator final : pal4::IMathSquareRootEvaluator {
    std::uint32_t calls = 0u;

    double SquareRoot(const double value) override {
        ++calls;
        return std::sqrt(value);
    }
};

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    pal4::MathRuntimeSquareRootTable table;
    table.Initialize();
    Check(table.EvenExponents()[0] == 0u &&
              table.OddExponents()[0] == 0x6a09eu &&
              table.EvenExponents()[1] != 0u &&
              table.OddExponents()[1] >= table.OddExponents()[0],
          "two 8192-entry halves encode square-root mantissas");
    Check(std::bit_cast<std::uint64_t>(table.Approximate(0.0)) == 0u &&
              std::bit_cast<std::uint64_t>(table.Approximate(-0.0)) == 0u &&
              table.Approximate(1.0) == 1.0 &&
              table.Approximate(4.0) == 2.0 &&
              std::bit_cast<std::uint64_t>(table.Approximate(2.0)) ==
                  0x3ff6a09e00000000ull,
          "zero branch, exponent parity and table lookup");

    constexpr double input = 1.1;
    const auto output_bits = std::bit_cast<std::uint64_t>(
        table.Approximate(input));
    Check((output_bits & 0xffffffffu) ==
              (std::bit_cast<std::uint64_t>(input) & 0xffffffffu) &&
              std::bit_cast<double>(output_bits) > 1.0 &&
              std::bit_cast<double>(output_bits) < input,
          "lookup preserves the original double low word");
    Check(std::bit_cast<std::uint64_t>(
              pal4::MathSquareRoot::Approximate(2.0f, table)) ==
              0x3ff6a09e00000000ull &&
              pal4::MathSquareRoot::ReciprocalApproximate(4.0f, table) == 0.5,
          "game square-root wrappers reach the runtime lookup table");

    RecordingEvaluator evaluator;
    table.Initialize(evaluator);
    Check(evaluator.calls == 0x4000u,
          "initializer calls the square-root boundary for every table entry");
    table.Initialize(evaluator);
    Check(evaluator.calls == 0x8000u && table.EvenExponents()[0] == 0u,
          "runtime initializer rebuilds tables without a ready guard");

    pal4::GameMathTables game_tables;
    Check(game_tables.CalculateSquareRootTable() == 0u,
          "independent game math table initializes");
    for (std::uint32_t index = 0u;
         index < pal4::MathRuntimeSquareRootTable::kEntriesPerHalf; ++index) {
        Check(table.EvenExponents()[index] == game_tables.square_root_one[index] &&
                  table.OddExponents()[index] == game_tables.square_root_two[index],
              "both IDA table generators use the same mantissa layout");
    }
}
