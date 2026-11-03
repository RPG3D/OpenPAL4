#include "pal4/game_math_tables.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <fstream>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

void ConstantsAndWrapper() {
    pal4::GameMathTables tables;
    tables.three_half_pi = -1.0f;
    tables.InitializeConstants();
    constexpr unsigned expected = 0x4096cbe4u;
    Check(std::bit_cast<unsigned>(tables.three_half_pi) == expected,
          "constant setter stores one float result");
    tables.three_half_pi = 0.0f;
    tables.InitializeConstantsWrapper();
    Check(std::bit_cast<unsigned>(tables.three_half_pi) == expected,
          "wrapper forwards to constant setter");
    Check(tables.vector_three_half_pi == 0.0f,
          "first constant slot does not initialize vector slot");
    tables.InitializeVectorConstants();
    Check(std::bit_cast<unsigned>(tables.vector_three_half_pi) == expected,
          "second initializer writes vector global slot");
    tables.vector_three_half_pi = 0.0f;
    tables.InitializeVectorConstantsWrapper();
    Check(std::bit_cast<unsigned>(tables.vector_three_half_pi) == expected,
          "second thunk forwards to distinct constant setter");
}

void SquareRootTables() {
    pal4::GameMathTables tables;
    Check(tables.CalculateSquareRootTable() == 0 &&
          tables.square_root_ready &&
          tables.square_root_one[0] == 0 &&
          tables.square_root_two[0] == 0x6a09eu &&
          tables.square_root_one[1] != 0 &&
          tables.square_root_two[1] >= tables.square_root_two[0] &&
          tables.square_root_one.back() > tables.square_root_one[1],
          "both 8192-entry square root tables use double high-bit inputs");
    // Original runtime 93CFE8/944FE8, captured in M08 pair32 after 667F90.
    // Double-only sqrt generates 0x3f/0x7f/... for the first samples.
    Check(tables.square_root_one[1]==64u && tables.square_root_one[2]==128u &&
          tables.square_root_one[5]==320u && tables.square_root_one[123]==7842u &&
          tables.square_root_one[4095]==235609u && tables.square_root_one[8191]==434289u &&
          tables.square_root_two[1]==434424u && tables.square_root_two[9]==435148u &&
          tables.square_root_two[4096]==767610u && tables.square_root_two[8191]==1048512u,
          "original PC24 sqrt rounding precedes high-word table extraction");
    tables.square_root_one[0] = 0x123;
    Check(tables.CalculateSquareRootTable() == 1 &&
          tables.square_root_one[0] == 0x123,
          "ready flag skips table regeneration");
}

void SquareRootLookup() {
    pal4::GameMathTables tables;
    (void)tables.CalculateSquareRootTable();
    Check(std::bit_cast<std::uint64_t>(
              tables.ApproximateSquareRoot(0.0f)) == 0 &&
          tables.ApproximateSquareRoot(1.0f) == 1.0 &&
          tables.ApproximateSquareRoot(4.0f) == 2.0 &&
          std::bit_cast<std::uint64_t>(
              tables.ApproximateSquareRoot(2.0f)) ==
              0x3ff6a09e00000000ull,
          "zero and exact power inputs plus second table lookup");
    const auto input = static_cast<double>(1.1f);
    const auto output = tables.ApproximateSquareRoot(1.1f);
    Check((std::bit_cast<std::uint64_t>(output) & 0xffffffffu) ==
              (std::bit_cast<std::uint64_t>(input) & 0xffffffffu) &&
          output > 1.0 && output < 1.1,
          "lookup preserves converted input low word");
}

void AngleFolding() {
    pal4::GameMathTables tables;
    tables.InitializeConstants();
    std::uint8_t folded = 7;
    const auto near = [](double actual, double expected) {
        return std::abs(actual - expected) < 0.000001;
    };
    Check(near(tables.NormalizeAngle(0.5f, folded), 0.5) && folded == 7,
          "first-quadrant angle leaves flag untouched");
    Check(near(tables.NormalizeAngle(3.1415927f, folded), 0.0) &&
          folded == 1, "pi boundary folds and sets flag");
    folded = 7;
    Check(near(tables.NormalizeAngle(6.2831855f, folded), 0.0) &&
          folded == 7, "full turn wraps without setting flag");
    Check(near(tables.NormalizeAngle(7.0f, folded),
               7.0 - 6.2831855) && folded == 7,
          "positive angle above one turn is reduced");
    folded = 7;
    Check(near(tables.NormalizeAngle(2.0f, folded),
               3.1415927 - 2.0) && folded == 1,
          "positive second quadrant folds and sets flag");
    folded = 7;
    Check(near(tables.NormalizeAngle(5.5f, folded),
               6.2831855 - 5.5) && folded == 7,
          "positive fourth quadrant folds without flag");
    folded = 7;
    Check(near(tables.NormalizeAngle(-3.1415927f, folded), 0.0) &&
          folded == 1, "negative pi boundary folds and sets flag");
    folded = 7;
    Check(near(tables.NormalizeAngle(-6.2831855f, folded), 0.0) &&
          folded == 7, "negative full turn wraps without setting flag");
    folded = 7;
    Check(near(tables.NormalizeAngle(-2.0f, folded),
               3.1415927 - 2.0) && folded == 1,
          "negative second quadrant folds and sets flag");
    folded = 7;
    Check(near(tables.NormalizeAngle(-5.5f, folded),
               6.2831855 - 5.5) && folded == 7,
          "negative fourth quadrant folds without flag");
}

void ApproximateNormalizationAndCosine() {
    pal4::GameMathTables tables;
    tables.InitializeConstants();
    (void)tables.CalculateSquareRootTable();
    Check(std::bit_cast<unsigned>(static_cast<float>(tables.ApproximateInverseSquareRoot(1.0f)))==0x3f7ffffeu &&
          tables.ApproximateInverseSquareRoot(4.0f)==0.5 &&
          tables.ApproximateInverseSquareRoot(0.0f)==8388608.0,
          "PC24 inverse lookup rounds epsilon sum and reciprocal; 2+epsilon ties to2");
    Check(tables.ApproximateCosine(0.0f) == 1.0 &&
          std::abs(tables.ApproximateCosine(3.1415927f) + 1.0) < 0.000001 &&
          std::abs(tables.ApproximateCosine(6.2831855f) - 1.0) < 0.000001 &&
          std::abs(tables.ApproximateCosine(1.5707964f)) < 0.01 &&
          std::abs(tables.ApproximateCosine(-2.0f) -
                   tables.ApproximateCosine(2.0f)) < 0.000001,
          "cosine polynomial respects quadrant folding and even symmetry");
}

void InverseTrigonometricApproximations() {
    pal4::GameMathTables tables;
    Check(tables.ApproximateArcsine(2.0f) == 0.0 &&
          tables.ApproximateArcsine(-2.0f) == 0.0 &&
          tables.ApproximateArccosine(2.0f) == 0.0 &&
          tables.ApproximateArccosine(-2.0f) == 0.0,
          "both inverse functions reject values beyond unit range");
    Check(std::abs(tables.ApproximateArcsine(1.0f) -
                   1.5707964) < 0.000001 &&
          std::abs(tables.ApproximateArcsine(-1.0f) +
                   1.5707964) < 0.000001 &&
          tables.ApproximateArccosine(1.0f) == 0.0 &&
          std::abs(tables.ApproximateArccosine(-1.0f) -
                   3.1415927) < 0.000001,
          "unit endpoints retain sign and pi constants");
    Check(std::abs(tables.ApproximateArcsine(0.5f) -
                   std::asin(0.5)) < 0.001 &&
          std::abs(tables.ApproximateArcsine(-0.5f) +
                   tables.ApproximateArcsine(0.5f)) < 0.000001 &&
          std::abs(tables.ApproximateArccosine(-0.5f) +
                   tables.ApproximateArccosine(0.5f) - 3.1415927) < 0.000001,
          "shared polynomial gives inverse sine and cosine symmetry");
}

}  // namespace

int main(int argc,char** argv) {
    ConstantsAndWrapper();
    SquareRootTables();
    SquareRootLookup();
    AngleFolding();
    ApproximateNormalizationAndCosine();
    // Original QN05 pair109: identical movement seed3EF71E6F/0/3F17F128,
    // normalized cosine3F469ED0; first actual walk-frame yaw421C7935.
    {
        pal4::GameMathTables tables;
        const auto radians=tables.ApproximateArccosine(std::bit_cast<float>(0x3f469ed0u));
        const auto yaw=static_cast<float>(radians*57.295780181884766);
        Check(std::bit_cast<unsigned>(yaw)==0x421c7935u,
              "original QN05 first walk frame keeps PC24 polynomial rounding");
    }
    InverseTrigonometricApproximations();
    if(argc==3 && std::string_view(argv[1])=="--dump-sqrt-table"){
        pal4::GameMathTables tables;(void)tables.CalculateSquareRootTable();
        std::ofstream out(argv[2],std::ios::binary);
        out.write(reinterpret_cast<const char*>(tables.square_root_one.data()),sizeof(tables.square_root_one));
        out.write(reinterpret_cast<const char*>(tables.square_root_two.data()),sizeof(tables.square_root_two));
        Check(bool(out),"write bounded generated sqrt-table diagnostic");
    }else Check(argc==1,"usage: math tables test [--dump-sqrt-table OUTPUT]");
}
