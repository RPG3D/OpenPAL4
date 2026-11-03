#pragma once

#include "pal4/math_lane_record.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct alignas(16) MathSimdRegister final {
    std::array<std::uint32_t, 4u> lanes{};

    constexpr MathSimdRegister(
        const std::uint32_t lane0 = 0u,
        const std::uint32_t lane1 = 0u,
        const std::uint32_t lane2 = 0u,
        const std::uint32_t lane3 = 0u)
        : lanes{lane0, lane1, lane2, lane3} {}

    [[nodiscard]] constexpr std::uint32_t& operator[](
        const std::uint32_t index) { return lanes[index]; }
    [[nodiscard]] constexpr const std::uint32_t& operator[](
        const std::uint32_t index) const { return lanes[index]; }
};

static_assert(sizeof(MathSimdRegister) == 16);
static_assert(alignof(MathSimdRegister) == 16);

class IMathTrigonometryRuntime {
public:
    virtual ~IMathTrigonometryRuntime() = default;
    virtual void ApproximateSine(MathLaneRecord& lanes) = 0;
    virtual void ApproximateCosine(MathLaneRecord& lanes) = 0;
    virtual void ApproximateSineCosine(
        const MathSimdRegister& xmm2, const MathSimdRegister& xmm6,
        MathLaneRecord& sine_lanes, MathLaneRecord& cosine_lanes) = 0;
    virtual void ApproximateVectorPair(
        const MathSimdRegister& input, MathSimdRegister& sine,
        MathSimdRegister& cosine) = 0;
    virtual void ApproximateTangent(
        const MathSimdRegister& xmm4, MathLaneRecord& lanes) = 0;
    virtual void ApproximateArctangent(MathLaneRecord& lanes) = 0;
};

class MathTrigonometryRuntimeStub final : public IMathTrigonometryRuntime {
public:
    void ApproximateSine(MathLaneRecord&) override {}
    void ApproximateCosine(MathLaneRecord&) override {}
    void ApproximateSineCosine(const MathSimdRegister&,
                              const MathSimdRegister&,
                              MathLaneRecord&,
                              MathLaneRecord&) override {}
    void ApproximateVectorPair(const MathSimdRegister&,
                               MathSimdRegister&,
                               MathSimdRegister&) override {}
    void ApproximateTangent(const MathSimdRegister&,
                            MathLaneRecord&) override {}
    void ApproximateArctangent(MathLaneRecord&) override {}
};

class MathTrigonometry final {
public:
    // IDA 0x601a00 and tail 0x792e43.
    static void ScalarSine(float radians, float& output);
    // IDA 0x601a10.
    [[nodiscard]] static float* ApproximateSine(
        std::uint32_t input_bits, float* output,
        MathLaneRecord& shared_lanes, IMathTrigonometryRuntime& runtime);
    // IDA 0x601a50 and tail 0x792e50.
    static void ScalarCosine(float radians, float& output);
    // IDA 0x601a60.
    [[nodiscard]] static float* ApproximateCosine(
        std::uint32_t input_bits, float* output,
        MathLaneRecord& shared_lanes, IMathTrigonometryRuntime& runtime);
    // IDA 0x601aa0, which jumps to the separate 0x792e5d entry.
    [[nodiscard]] static float* ScalarSineCosine(float radians, float& sine,
                                                  float& cosine);
    // IDA 0x601ab0.
    [[nodiscard]] static float* ApproximateSineCosine(
        std::uint32_t input_bits, float* sine, float* cosine,
        const MathSimdRegister& xmm2, const MathSimdRegister& xmm6,
        MathLaneRecord& sine_lanes, MathLaneRecord& cosine_lanes,
        IMathTrigonometryRuntime& runtime);
    // IDA 0x601b00.
    [[nodiscard]] static float* ScalarSineCosineFour(
        const float* radians, float* sine, float* cosine);
    // IDA 0x601b60, thunk to the separate 0x792923 entry.
    [[nodiscard]] static MathSimdRegister* ApproximateVectorPair(
        const MathSimdRegister& input, MathSimdRegister& sine,
        MathSimdRegister& cosine, IMathTrigonometryRuntime& runtime);
    // IDA 0x601b70/0x601b80/0x601b90.
    static void ScalarArcsine(float value, float& output);
    static void ScalarArccosine(float value, float& output);
    static void ScalarTangent(float radians, float& output);
    // IDA 0x601ba0: SSE replacement for scalar tangent slot 6.
    [[nodiscard]] static float* ApproximateTangent(
        std::uint32_t input_bits, float* output,
        const MathSimdRegister& xmm4, MathLaneRecord& shared_lanes,
        IMathTrigonometryRuntime& runtime);
    // IDA 0x601be0.
    static void ScalarArctangent(float value, float& output);
    // IDA 0x601bf0: SSE replacement for scalar arctangent slot 7.
    [[nodiscard]] static float* ApproximateArctangent(
        std::uint32_t input_bits, float* output,
        MathLaneRecord& shared_lanes, IMathTrigonometryRuntime& runtime);
};

}  // namespace pal4
