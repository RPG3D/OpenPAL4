#include "pal4/math_trigonometry.h"

#include <bit>
#include <cmath>

namespace pal4 {

void MathTrigonometry::ScalarSine(const float radians, float& output) {
    output = static_cast<float>(std::sin(static_cast<double>(radians)));
}

float* MathTrigonometry::ApproximateSine(
    const std::uint32_t input_bits, float* const output,
    MathLaneRecord& shared_lanes, IMathTrigonometryRuntime& runtime) {
    (void)MathLaneRecordWriter::Fill(
        shared_lanes, std::bit_cast<float>(input_bits),
        input_bits, input_bits, input_bits);
    runtime.ApproximateSine(shared_lanes);
    *output = shared_lanes.value;
    return output;
}

void MathTrigonometry::ScalarCosine(const float radians, float& output) {
    output = static_cast<float>(std::cos(static_cast<double>(radians)));
}

float* MathTrigonometry::ApproximateCosine(
    const std::uint32_t input_bits, float* const output,
    MathLaneRecord& shared_lanes, IMathTrigonometryRuntime& runtime) {
    (void)MathLaneRecordWriter::Fill(
        shared_lanes, std::bit_cast<float>(input_bits),
        input_bits, input_bits, input_bits);
    runtime.ApproximateCosine(shared_lanes);
    *output = shared_lanes.value;
    return output;
}

float* MathTrigonometry::ScalarSineCosine(
    const float radians, float& sine, float& cosine) {
    const auto angle = static_cast<double>(radians);
    cosine = static_cast<float>(std::cos(angle));
    sine = static_cast<float>(std::sin(angle));
    return &sine;
}

float* MathTrigonometry::ApproximateSineCosine(
    const std::uint32_t input_bits, float* const sine,
    float* const cosine, const MathSimdRegister& xmm2,
    const MathSimdRegister& xmm6, MathLaneRecord& sine_lanes,
    MathLaneRecord& cosine_lanes,
    IMathTrigonometryRuntime& runtime) {
    (void)MathLaneRecordWriter::Fill(
        sine_lanes, std::bit_cast<float>(input_bits),
        input_bits, input_bits, input_bits);
    runtime.ApproximateSineCosine(xmm2, xmm6, sine_lanes, cosine_lanes);
    *sine = sine_lanes.value;
    *cosine = cosine_lanes.value;
    return sine;
}

float* MathTrigonometry::ScalarSineCosineFour(
    const float* const radians, float* const sine,
    float* const cosine) {
    for (std::uint32_t lane = 0u; lane < 4u; ++lane) {
        (void)ScalarSineCosine(radians[lane], sine[lane], cosine[lane]);
    }
    return sine + 3u;
}

MathSimdRegister* MathTrigonometry::ApproximateVectorPair(
    const MathSimdRegister& input, MathSimdRegister& sine,
    MathSimdRegister& cosine, IMathTrigonometryRuntime& runtime) {
    runtime.ApproximateVectorPair(input, sine, cosine);
    return &sine;
}

void MathTrigonometry::ScalarArcsine(
    const float value, float& output) {
    output = static_cast<float>(std::asin(static_cast<double>(value)));
}

void MathTrigonometry::ScalarArccosine(
    const float value, float& output) {
    output = static_cast<float>(std::acos(static_cast<double>(value)));
}

void MathTrigonometry::ScalarTangent(
    const float radians, float& output) {
    const auto angle = static_cast<double>(radians);
    output = static_cast<float>(std::sin(angle) / std::cos(angle));
}

float* MathTrigonometry::ApproximateTangent(
    const std::uint32_t input_bits, float* const output,
    const MathSimdRegister& xmm4, MathLaneRecord& shared_lanes,
    IMathTrigonometryRuntime& runtime) {
    (void)MathLaneRecordWriter::Fill(
        shared_lanes, std::bit_cast<float>(input_bits),
        input_bits, input_bits, input_bits);
    runtime.ApproximateTangent(xmm4, shared_lanes);
    *output = shared_lanes.value;
    return output;
}

void MathTrigonometry::ScalarArctangent(
    const float value, float& output) {
    output = static_cast<float>(std::atan(static_cast<double>(value)));
}

float* MathTrigonometry::ApproximateArctangent(
    const std::uint32_t input_bits, float* const output,
    MathLaneRecord& shared_lanes,
    IMathTrigonometryRuntime& runtime) {
    (void)MathLaneRecordWriter::Fill(
        shared_lanes, std::bit_cast<float>(input_bits),
        input_bits, input_bits, input_bits);
    runtime.ApproximateArctangent(shared_lanes);
    *output = shared_lanes.value;
    return output;
}

}  // namespace pal4
