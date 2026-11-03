#include "pal4/math_simd_constants.h"

namespace pal4 {

void MathSimdConstants::Fill(Lanes& lanes, const double value) {
    lanes.fill(static_cast<float>(value));
}

void MathSimdConstants::InitializePiWrapper() { InitializePi(); }
void MathSimdConstants::InitializePi() {
    Fill(pi_, 3.141592653589793);
}

void MathSimdConstants::InitializeHalfPiWrapper() { InitializeHalfPi(); }
void MathSimdConstants::InitializeHalfPi() {
    Fill(half_pi_, 1.5707963267948966);
}

void MathSimdConstants::InitializeTwoOverPiWrapper() { InitializeTwoOverPi(); }
void MathSimdConstants::InitializeTwoOverPi() {
    Fill(two_over_pi_, 0.6366197723675814);
}

void MathSimdConstants::InitializeQuarterPiWrapper() { InitializeQuarterPi(); }
void MathSimdConstants::InitializeQuarterPi() {
    Fill(quarter_pi_, 0.7853981633974483);
}

void MathSimdConstants::InitializeFourOverPiWrapper() { InitializeFourOverPi(); }
void MathSimdConstants::InitializeFourOverPi() {
    Fill(four_over_pi_, 1.2732395447351628);
}

}  // namespace pal4
