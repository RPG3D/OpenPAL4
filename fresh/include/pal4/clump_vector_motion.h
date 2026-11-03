#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct ClumpMotionVector final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ClumpVectorMotionState final {
    ClumpMotionVector velocity;
    ClumpMotionVector start;
    ClumpMotionVector end;
    float launch_value = 0.0f;
    float gravity = 0.0f;
    std::uint32_t status = 0u;
};

static_assert(sizeof(ClumpVectorMotionState) == 48);
static_assert(offsetof(ClumpVectorMotionState, start) == 12);
static_assert(offsetof(ClumpVectorMotionState, end) == 24);
static_assert(offsetof(ClumpVectorMotionState, launch_value) == 36);
static_assert(offsetof(ClumpVectorMotionState, gravity) == 40);
static_assert(offsetof(ClumpVectorMotionState, status) == 44);

class IClumpVectorMotionRuntime {
public:
    virtual ~IClumpVectorMotionRuntime() = default;
    virtual void Normalize(ClumpMotionVector& vector) = 0;
    [[nodiscard]] virtual double ApproximateMagnitude(
        const ClumpMotionVector& vector) = 0;
};

class ClumpVectorMotionRuntimeStub final : public IClumpVectorMotionRuntime {
public:
    void Normalize(ClumpMotionVector&) override {}
    [[nodiscard]] double ApproximateMagnitude(
        const ClumpMotionVector&) override { return 0.0; }
};

class ClumpVectorMotion final {
public:
    // IDA 0x5fb810.
    [[nodiscard]] static ClumpMotionVector& Initialize(
        ClumpVectorMotionState& state, ClumpMotionVector& output,
        const ClumpMotionVector& start, const ClumpMotionVector& end,
        float launch_value, float gravity,
        IClumpVectorMotionRuntime& runtime);
    // IDA 0x5fb9a0.
    [[nodiscard]] static ClumpMotionVector& ScaleAndSetStatus(
        ClumpVectorMotionState& state, ClumpMotionVector& output,
        float scale);
};

}  // namespace pal4
