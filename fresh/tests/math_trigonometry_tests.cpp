#include "pal4/math_trigonometry.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IMathTrigonometryRuntime {
    bool sine_called = false;
    bool cosine_called = false;
    bool pair_called = false;
    bool vector_pair_called = false;
    bool tangent_called = false;
    bool atan_called = false;

    void ApproximateSine(pal4::MathLaneRecord& lanes) override {
        Check(std::bit_cast<std::uint32_t>(lanes.value) ==
                  0x3f800000u &&
                  lanes.lane1 == 0x3f800000u &&
                  lanes.lane2 == 0x3f800000u &&
                  lanes.lane3 == 0x3f800000u,
              "SIMD approximation receives four copies of input bits");
        sine_called = true;
        lanes.value = 0.75f;
    }
    void ApproximateCosine(pal4::MathLaneRecord& lanes) override {
        Check(lanes.lane1 == 0x3f800000u &&
                  lanes.lane2 == 0x3f800000u &&
                  lanes.lane3 == 0x3f800000u,
              "cosine approximation receives replicated input bits");
        cosine_called = true;
        lanes.value = 0.25f;
    }
    void ApproximateSineCosine(
        const pal4::MathSimdRegister& xmm2,
        const pal4::MathSimdRegister& xmm6,
        pal4::MathLaneRecord& sine_lanes,
        pal4::MathLaneRecord& cosine_lanes) override {
        Check(xmm2[0u] == 0x11111111u &&
                  xmm6[0u] == 0x22222222u &&
                  sine_lanes.lane1 == 0x3f800000u &&
                  sine_lanes.lane2 == 0x3f800000u &&
                  sine_lanes.lane3 == 0x3f800000u,
              "pair approximation receives SIMD registers and shared lanes");
        pair_called = true;
        sine_lanes.value = 0.5f;
        cosine_lanes.value = -0.5f;
    }
    void ApproximateVectorPair(
        const pal4::MathSimdRegister& input,
        pal4::MathSimdRegister& sine,
        pal4::MathSimdRegister& cosine) override {
        Check(input[0u] == 0x3f800000u &&
                  input[3u] == 0x40000000u,
              "vector thunk forwards all input lanes");
        vector_pair_called = true;
        sine[0u] = 0x11111111u;
        cosine[3u] = 0x22222222u;
    }
    void ApproximateTangent(
        const pal4::MathSimdRegister& xmm4,
        pal4::MathLaneRecord& lanes) override {
        Check(xmm4[0u] == 0x11111111u &&
                  lanes.lane1 == 0x3f800000u &&
                  lanes.lane2 == 0x3f800000u &&
                  lanes.lane3 == 0x3f800000u,
              "tangent approximation receives XMM4 and replicated input");
        tangent_called = true;
        lanes.value = 0.125f;
    }
    void ApproximateArctangent(
        pal4::MathLaneRecord& lanes) override {
        Check(lanes.lane1 == 0x3f800000u &&
                  lanes.lane2 == 0x3f800000u &&
                  lanes.lane3 == 0x3f800000u,
              "acos approximation receives replicated input");
        atan_called = true;
        lanes.value = 0.375f;
    }
};

}  // namespace

int main() {
    float output = 0.0f;
    pal4::MathTrigonometry::ScalarSine(0.0f, output);
    Check(output == 0.0f, "scalar sine writes zero");
    pal4::MathTrigonometry::ScalarSine(
        std::numbers::pi_v<float> / 2.0f, output);
    Check(std::fabs(output - 1.0f) < 0.000001f,
          "scalar sine writes quarter-turn result");
    pal4::MathTrigonometry::ScalarSine(
        -std::numbers::pi_v<float> / 2.0f, output);
    Check(std::fabs(output + 1.0f) < 0.000001f,
          "scalar sine preserves negative input");

    pal4::MathLaneRecord lanes{};
    Runtime runtime;
    Check(pal4::MathTrigonometry::ApproximateSine(
              0x3f800000u, &output, lanes, runtime) == &output &&
              runtime.sine_called && output == 0.75f,
          "SIMD wrapper forwards changed first lane and output address");

    pal4::MathTrigonometry::ScalarCosine(0.0f, output);
    Check(output == 1.0f, "scalar cosine writes one at zero");
    pal4::MathTrigonometry::ScalarCosine(
        std::numbers::pi_v<float>, output);
    Check(std::fabs(output + 1.0f) < 0.000001f,
          "scalar cosine writes half-turn result");
    Check(pal4::MathTrigonometry::ApproximateCosine(
              0x3f800000u, &output, lanes, runtime) == &output &&
              runtime.cosine_called && output == 0.25f,
          "cosine SIMD wrapper copies first result lane");

    float cosine = 9.0f;
    Check(pal4::MathTrigonometry::ScalarSineCosine(
              0.0f, output, cosine) == &output &&
              output == 0.0f && cosine == 1.0f,
          "fsincos wrapper returns sine pointer and writes both outputs");
    Check(pal4::MathTrigonometry::ScalarSineCosine(
              0.5f, output, output) == &output &&
              std::fabs(output - std::sin(0.5f)) < 0.000001f,
          "sine store wins when output pointers alias");
    const pal4::MathSimdRegister xmm2{0x11111111u, 0u, 0u, 0u};
    const pal4::MathSimdRegister xmm6{0x22222222u, 0u, 0u, 0u};
    pal4::MathLaneRecord cosine_lanes{};
    Check(pal4::MathTrigonometry::ApproximateSineCosine(
              0x3f800000u, &output, &cosine, xmm2, xmm6,
              lanes, cosine_lanes, runtime) == &output &&
              runtime.pair_called && output == 0.5f &&
              cosine == -0.5f,
          "paired SIMD wrapper writes both results in native order");

    const float radians[4u] = {
        0.0f, std::numbers::pi_v<float> / 2.0f,
        std::numbers::pi_v<float>,
        -std::numbers::pi_v<float> / 2.0f};
    float sines[4u]{};
    float cosines[4u]{};
    Check(pal4::MathTrigonometry::ScalarSineCosineFour(
              radians, sines, cosines) == &sines[3u] &&
              std::fabs(sines[0u]) < 0.000001f &&
              std::fabs(sines[1u] - 1.0f) < 0.000001f &&
              std::fabs(sines[2u]) < 0.000001f &&
              std::fabs(sines[3u] + 1.0f) < 0.000001f &&
              std::fabs(cosines[0u] - 1.0f) < 0.000001f &&
              std::fabs(cosines[1u]) < 0.000001f &&
              std::fabs(cosines[2u] + 1.0f) < 0.000001f &&
              std::fabs(cosines[3u]) < 0.000001f,
          "four-lane scalar wrapper writes ordered sine and cosine arrays");

    const pal4::MathSimdRegister vector_input{
        0x3f800000u, 0u, 0u, 0x40000000u};
    pal4::MathSimdRegister vector_sine{};
    pal4::MathSimdRegister vector_cosine{};
    Check(pal4::MathTrigonometry::ApproximateVectorPair(
              vector_input, vector_sine, vector_cosine, runtime) ==
              &vector_sine && runtime.vector_pair_called &&
              vector_sine[0u] == 0x11111111u &&
              vector_cosine[3u] == 0x22222222u,
          "SIMD thunk forwards input and both output registers");

    pal4::MathTrigonometry::ScalarArcsine(0.5f, output);
    Check(std::fabs(output - std::numbers::pi_v<float> / 6.0f) <
              0.000001f,
          "arcsine computes inverse sine within domain");
    pal4::MathTrigonometry::ScalarArcsine(2.0f, output);
    Check(std::isnan(output), "arcsine propagates out-of-domain NaN");
    pal4::MathTrigonometry::ScalarArccosine(0.0f, output);
    Check(std::fabs(output - std::numbers::pi_v<float> / 2.0f) <
              0.000001f,
          "arccosine computes inverse cosine");
    pal4::MathTrigonometry::ScalarTangent(
        std::numbers::pi_v<float> / 4.0f, output);
    Check(std::fabs(output - 1.0f) < 0.000001f,
          "tangent divides scalar sine by cosine");

    Check(pal4::MathTrigonometry::ApproximateTangent(
              0x3f800000u, &output, xmm2, lanes, runtime) ==
              &output && runtime.tangent_called && output == 0.125f,
          "tangent SIMD wrapper forwards XMM4 and first result lane");
    pal4::MathTrigonometry::ScalarArctangent(1.0f, output);
    Check(std::fabs(output - std::numbers::pi_v<float> / 4.0f) <
              0.000001f,
          "scalar atan computes quarter turn");
    Check(pal4::MathTrigonometry::ApproximateArctangent(
              0x3f800000u, &output, lanes, runtime) ==
              &output && runtime.atan_called && output == 0.375f,
          "arctangent SIMD wrapper copies first result lane");
}
