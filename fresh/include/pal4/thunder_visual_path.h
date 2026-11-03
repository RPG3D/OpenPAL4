#pragma once

#include "pal4/thunder_object.h"
#include "pal4/thunder_renderer.h"
#include "pal4/thunder_visual_transform.h"

#include <cstdint>
#include <vector>

namespace pal4 {

struct ThunderSplineBufferRange final {
    std::uint32_t begin{}, end{}, capacity{};
};

class IThunderSplineBufferRuntime {
public:
    virtual ~IThunderSplineBufferRuntime() = default;
    virtual void Free(std::uint32_t address) = 0;
};

class ThunderSplineBuffers final {
public:
    // IDA 0x64b250/0x64b270, paired CRT atexit callbacks.
    static void CleanupSecond(ThunderSplineBufferRange& range,
                              IThunderSplineBufferRuntime& runtime);
    static void CleanupFirst(ThunderSplineBufferRange& range,
                             IThunderSplineBufferRuntime& runtime);
};

class IThunderVisualPathRuntime {
public:
    virtual ~IThunderVisualPathRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Random() = 0;
    virtual void WriteSegments(
        ThunderObjectState& state,
        const std::vector<ThunderVisualSegment>& segments) = 0;
    [[nodiscard]] virtual std::uint32_t CreateSpline(
        const std::vector<ThunderVector3>& points,
        std::uint32_t spline_type) = 0;
    [[nodiscard]] virtual ThunderVector3 SplinePosition(
        std::uint32_t spline, std::uint32_t mode, float position) = 0;
    [[nodiscard]] virtual std::uint8_t DestroySpline(
        std::uint32_t spline) = 0;
};

class ThunderVisualPathRuntimeStub final : public IThunderVisualPathRuntime {
public:
    [[nodiscard]] std::uint32_t Random() override { return 0u; }
    void WriteSegments(ThunderObjectState&,
                       const std::vector<ThunderVisualSegment>&) override {}
    [[nodiscard]] std::uint32_t CreateSpline(
        const std::vector<ThunderVector3>&,
        std::uint32_t) override { return 0u; }
    [[nodiscard]] ThunderVector3 SplinePosition(
        std::uint32_t, std::uint32_t, float) override { return {}; }
    [[nodiscard]] std::uint8_t DestroySpline(
        std::uint32_t) override { return 0u; }
};

class ThunderVisualPath final {
public:
    // IDA 0x64b290.
    static void TransformToWorld(
        ThunderObjectState& state,
        class IThunderPathTransformRuntime& runtime);

    // IDA 0x64ab40. Native AL outside the spline path is the sample flag.
    [[nodiscard]] static std::uint8_t Build(
        ThunderObjectState& state, IThunderVisualPathRuntime& runtime);
};

class ThunderSegmentRecords final {
public:
    // IDA 0x64b470: copy each 24-byte endpoint pair and return destination end.
    [[nodiscard]] static ThunderVisualSegment* Copy(
        const ThunderVisualSegment* source_begin,
        const ThunderVisualSegment* source_end,
        ThunderVisualSegment* destination);
};

class IThunderPathTransformRuntime {
public:
    virtual ~IThunderPathTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::vector<ThunderVisualSegment> ReadSegments(
        std::uint32_t begin, std::uint32_t end) = 0;
    [[nodiscard]] virtual ThunderVector3 TransformVector(
        ThunderVector3 value, std::uint32_t transform) = 0;
    virtual void WriteSegments(
        ThunderObjectState& state,
        const std::vector<ThunderVisualSegment>& segments) = 0;
};

class ThunderPathTransformRuntimeStub final
    : public IThunderPathTransformRuntime {
public:
    [[nodiscard]] std::uint32_t FrameTransform(
        std::uint32_t) override { return 0u; }
    [[nodiscard]] std::vector<ThunderVisualSegment> ReadSegments(
        std::uint32_t, std::uint32_t) override { return {}; }
    [[nodiscard]] ThunderVector3 TransformVector(
        ThunderVector3 value, std::uint32_t) override { return value; }
    void WriteSegments(ThunderObjectState&,
                       const std::vector<ThunderVisualSegment>&) override {}
};

}  // namespace pal4
