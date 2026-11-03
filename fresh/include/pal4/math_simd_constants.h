#pragma once

#include <array>

namespace pal4 {

class MathSimdConstants final {
public:
    using Lanes = std::array<float, 4>;

    void InitializePiWrapper();
    void InitializePi();
    void InitializeHalfPiWrapper();
    void InitializeHalfPi();
    void InitializeTwoOverPiWrapper();
    void InitializeTwoOverPi();
    void InitializeQuarterPiWrapper();
    void InitializeQuarterPi();
    void InitializeFourOverPiWrapper();
    void InitializeFourOverPi();

    const Lanes& Pi() const { return pi_; }
    const Lanes& HalfPi() const { return half_pi_; }
    const Lanes& TwoOverPi() const { return two_over_pi_; }
    const Lanes& QuarterPi() const { return quarter_pi_; }
    const Lanes& FourOverPi() const { return four_over_pi_; }

private:
    static void Fill(Lanes& lanes, double value);

    Lanes pi_{};
    Lanes half_pi_{};
    Lanes two_over_pi_{};
    Lanes quarter_pi_{};
    Lanes four_over_pi_{};
};

}  // namespace pal4
