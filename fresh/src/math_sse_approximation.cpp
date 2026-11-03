#include "pal4/math_sse_approximation.h"

#include <bit>
#include <cstdint>
#include <emmintrin.h>

namespace pal4 {

namespace {

__m128 Scalar(const std::uint32_t bits) {
    return _mm_set_ss(std::bit_cast<float>(bits));
}

float Evaluate(const __m128 scaled, const std::uint32_t sign) {
    const auto quadrant = _mm_cvttss_si32(scaled);
    const auto quadrant_float = _mm_cvtsi32_ss(_mm_setzero_ps(), quadrant);
    auto fraction = _mm_sub_ss(scaled, quadrant_float);
    fraction = _mm_min_ss(fraction, Scalar(0x3f800000u));

    const auto odd = (quadrant & 1) != 0;
    const auto one_minus = _mm_sub_ss(Scalar(0x3f800000u), fraction);
    const auto angle = odd ? one_minus : fraction;
    const auto square = _mm_mul_ss(angle, angle);

    auto polynomial = _mm_mul_ss(square, Scalar(0xbb996966u));
    polynomial = _mm_add_ss(polynomial, Scalar(0x3da335e3u));
    polynomial = _mm_mul_ss(polynomial, square);
    polynomial = _mm_add_ss(polynomial, Scalar(0xbf255de7u));
    polynomial = _mm_mul_ss(polynomial, square);
    polynomial = _mm_add_ss(polynomial, Scalar(0x3fc90fdbu));

    const auto angle_bits = std::bit_cast<std::uint32_t>(_mm_cvtss_f32(angle));
    const auto quadrant_sign = static_cast<std::uint32_t>(quadrant) << 30;
    const auto signed_angle = Scalar(angle_bits ^ sign ^
                                      (quadrant_sign & 0x80000000u));
    return _mm_cvtss_f32(_mm_mul_ss(signed_angle, polynomial));
}

}  // namespace

float MathSseApproximation::Sine(const float radians) {
    const auto sign = std::bit_cast<std::uint32_t>(radians) & 0x80000000u;
    auto scaled = _mm_mul_ss(_mm_set_ss(radians), Scalar(0x3f22f983u));
    scaled = _mm_and_ps(scaled, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffffu)));
    return Evaluate(scaled, sign);
}

float MathSseApproximation::Cosine(const float radians) {
    auto absolute = _mm_and_ps(_mm_set_ss(radians),
                               _mm_castsi128_ps(_mm_set1_epi32(0x7fffffffu)));
    absolute = _mm_add_ss(absolute, Scalar(0x3fc90fdbu));
    const auto scaled = _mm_mul_ss(absolute, Scalar(0x3f22f983u));
    return Evaluate(scaled, 0u);
}

}  // namespace pal4
