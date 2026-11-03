#include "pal4/math_square_root.h"

#include <xmmintrin.h>

namespace pal4 {

double MathSquareRoot::Approximate(
    const float value, IMathSquareRootRuntime& runtime) {
    return runtime.ApproximateSquareRoot(
        static_cast<double>(value));
}

double MathSquareRoot::Sse(const float value) {
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(value)));
}

double MathSquareRoot::ReciprocalApproximate(
    const float value, IMathSquareRootRuntime& runtime) {
    return 1.0 / runtime.ApproximateSquareRoot(
        static_cast<double>(value));
}

double MathSquareRoot::ReciprocalSse(const float value) {
    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(value)));
}

}  // namespace pal4
