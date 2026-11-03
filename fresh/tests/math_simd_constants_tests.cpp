#include "pal4/math_simd_constants.h"

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool AllBits(const pal4::MathSimdConstants::Lanes& lanes,
             const std::uint32_t bits) {
    for (const float value : lanes) {
        if (std::bit_cast<std::uint32_t>(value) != bits) return false;
    }
    return true;
}

}  // namespace

int main() {
    pal4::MathSimdConstants constants;
    Check(AllBits(constants.TwoOverPi(), 0u), "zero-initialized lanes");

    constants.InitializePiWrapper();
    Check(AllBits(constants.Pi(), 0x40490fdbu) &&
              AllBits(constants.HalfPi(), 0u),
          "PI wrapper writes only its four lanes");
    constants.InitializeHalfPiWrapper();
    Check(AllBits(constants.HalfPi(), 0x3fc90fdbu),
          "half-PI wrapper rounds double to float");
    constants.InitializeTwoOverPiWrapper();
    Check(AllBits(constants.TwoOverPi(), 0x3f22f983u),
          "two-over-PI wrapper fills the SIMD multiplier");
    constants.InitializeQuarterPiWrapper();
    Check(AllBits(constants.QuarterPi(), 0x3f490fdbu),
          "quarter-PI wrapper fills its four lanes");
    constants.InitializeFourOverPiWrapper();
    Check(AllBits(constants.FourOverPi(), 0x3fa2f983u),
          "four-over-PI wrapper fills its four lanes");

    constants.InitializePi();
    constants.InitializeHalfPi();
    constants.InitializeTwoOverPi();
    constants.InitializeQuarterPi();
    constants.InitializeFourOverPi();
    Check(AllBits(constants.Pi(), 0x40490fdbu) &&
              AllBits(constants.HalfPi(), 0x3fc90fdbu) &&
              AllBits(constants.TwoOverPi(), 0x3f22f983u) &&
              AllBits(constants.QuarterPi(), 0x3f490fdbu) &&
              AllBits(constants.FourOverPi(), 0x3fa2f983u),
          "direct bodies match static-initializer wrappers");
}
