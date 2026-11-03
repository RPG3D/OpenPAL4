#pragma once

#include "pal4/math_square_root.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IMathSquareRootEvaluator {
public:
    virtual ~IMathSquareRootEvaluator() = default;
    [[nodiscard]] virtual double SquareRoot(double value) = 0;
};

class MathRuntimeSquareRootTable final : public IMathSquareRootRuntime {
public:
    static constexpr std::uint32_t kEntriesPerHalf = 0x2000u;
    using Half = std::array<std::uint32_t, kEntriesPerHalf>;

    // IDA 0x792EDE rebuilds both halves on every call.
    void Initialize();
    void Initialize(IMathSquareRootEvaluator& evaluator);
    // IDA 0x792F55 looks up the double mantissa and returns via x87.
    [[nodiscard]] double Approximate(double value) const;
    [[nodiscard]] double ApproximateSquareRoot(double value) override {
        return Approximate(value);
    }

    [[nodiscard]] const Half& EvenExponents() const { return even_; }
    [[nodiscard]] const Half& OddExponents() const { return odd_; }

private:
    Half even_{};
    Half odd_{};
};

}  // namespace pal4
