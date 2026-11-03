#include "pal4/math_square_root.h"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <type_traits>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMathSquareRootRuntime {
    std::vector<double> inputs;
    double result = 2.5;

    double ApproximateSquareRoot(const double value) override {
        inputs.push_back(value);
        return result;
    }
};

}  // namespace

int main() {
    static_assert(std::is_same_v<
                  decltype(pal4::MathSquareRoot::Sse(1.0f)), double>);
    static_assert(std::is_same_v<
                  decltype(pal4::MathSquareRoot::ReciprocalSse(1.0f)), double>);
    Runtime runtime;
    Check(pal4::MathSquareRoot::Approximate(
              9.0f, runtime) == 2.5 &&
              runtime.inputs == std::vector<double>{9.0},
          "approximate thunk converts float and forwards native result");
    Check(pal4::MathSquareRoot::ReciprocalApproximate(
              16.0f, runtime) == 0.4 &&
              runtime.inputs == std::vector<double>{9.0, 16.0},
          "reciprocal wrapper divides one by approximation result");
    runtime.result = 0.0;
    Check(std::isinf(pal4::MathSquareRoot::ReciprocalApproximate(
              0.0f, runtime)),
          "zero approximation produces reciprocal infinity");

    Check(pal4::MathSquareRoot::Sse(9.0f) == 3.0f &&
              pal4::MathSquareRoot::Sse(0.0f) == 0.0f &&
              std::isnan(pal4::MathSquareRoot::Sse(-1.0f)),
          "SSE sqrt matches normal, zero and negative instruction cases");
    Check(std::fabs(pal4::MathSquareRoot::ReciprocalSse(4.0f) -
                    0.5f) < 0.001f &&
              std::isinf(pal4::MathSquareRoot::ReciprocalSse(0.0f)),
          "SSE reciprocal sqrt preserves approximate finite and zero classes");
    const auto reciprocal = pal4::MathSquareRoot::ReciprocalSse(3.0f);
    Check(reciprocal == static_cast<double>(static_cast<float>(reciprocal)),
          "single-precision SSE result is widened through the x87 return");
}
