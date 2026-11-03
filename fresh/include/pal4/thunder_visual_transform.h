#pragma once

#include "pal4/thunder_object.h"

#include <array>
#include <cstdint>

namespace pal4 {

using ThunderVector3 = std::array<float, 3u>;

struct ThunderFrameMatrix final {
    ThunderVector3 right{};
    std::uint32_t flags{};
    ThunderVector3 up{};
    std::uint32_t pad_up{};
    ThunderVector3 at{};
    std::uint32_t pad_at{};
    ThunderVector3 position{};
    std::uint32_t pad_position{};
};
static_assert(sizeof(ThunderFrameMatrix) == 64u);

class IThunderVisualTransformRuntime {
public:
    virtual ~IThunderVisualTransformRuntime() = default;
    [[nodiscard]] virtual std::uint32_t FrameTransform(
        std::uint32_t frame) = 0;
    [[nodiscard]] virtual ThunderVector3 MatrixPosition(
        std::uint32_t transform) = 0;
    [[nodiscard]] virtual ThunderVector3 TransformPoint(
        ThunderVector3 point, std::uint32_t transform) = 0;
    [[nodiscard]] virtual ThunderFrameMatrix BuildTransformMatrix(
        std::uint32_t transform) = 0;
    virtual void MultiplyMatrix(ThunderFrameMatrix& matrix,
                                std::uint32_t frame_matrix) = 0;
    [[nodiscard]] virtual ThunderVector3 TransformPoint(
        ThunderVector3 point, const ThunderFrameMatrix& matrix) = 0;
    [[nodiscard]] virtual float VectorLength(ThunderVector3 value) = 0;
    [[nodiscard]] virtual ThunderVector3 Normalize(ThunderVector3 value) = 0;
    [[nodiscard]] virtual std::uint32_t CameraFrame() = 0;
    virtual void MatrixUpdate(ThunderFrameMatrix& matrix) = 0;
    virtual void FrameSetIdentity(std::uint32_t frame) = 0;
    virtual void ApplyMatrix(std::uint32_t frame,
                             const ThunderFrameMatrix& matrix,
                             std::uint32_t combine) = 0;
    [[nodiscard]] virtual std::uint32_t FrameUpdateObjects(
        std::uint32_t frame) = 0;
};

class ThunderVisualTransformRuntimeStub final
    : public IThunderVisualTransformRuntime {
public:
    [[nodiscard]] std::uint32_t FrameTransform(std::uint32_t) override {
        return 0u;
    }
    [[nodiscard]] ThunderVector3 MatrixPosition(std::uint32_t) override {
        return {};
    }
    [[nodiscard]] ThunderVector3 TransformPoint(
        ThunderVector3, std::uint32_t) override { return {}; }
    [[nodiscard]] ThunderFrameMatrix BuildTransformMatrix(
        std::uint32_t) override { return {}; }
    void MultiplyMatrix(ThunderFrameMatrix&, std::uint32_t) override {}
    [[nodiscard]] ThunderVector3 TransformPoint(
        ThunderVector3, const ThunderFrameMatrix&) override { return {}; }
    [[nodiscard]] float VectorLength(ThunderVector3) override { return 0.0f; }
    [[nodiscard]] ThunderVector3 Normalize(ThunderVector3) override {
        return {};
    }
    [[nodiscard]] std::uint32_t CameraFrame() override { return 0u; }
    void MatrixUpdate(ThunderFrameMatrix&) override {}
    void FrameSetIdentity(std::uint32_t) override {}
    void ApplyMatrix(std::uint32_t, const ThunderFrameMatrix&,
                     std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameUpdateObjects(
        std::uint32_t) override { return 0u; }
};

class ThunderVisualTransform final {
public:
    // IDA 0x64b350. Inactive native EAX is incidental.
    [[nodiscard]] static std::uint32_t SetupSourceFrame(
        ThunderObjectState& state,
        class IThunderSourceFrameRuntime& runtime);

    // IDA 0x64a850.
    [[nodiscard]] static std::uint32_t Update(
        ThunderObjectState& state, IThunderVisualTransformRuntime& runtime);
};

class IThunderSourceFrameRuntime {
public:
    virtual ~IThunderSourceFrameRuntime() = default;
    virtual void FrameSetIdentity(std::uint32_t frame) = 0;
    virtual void ScaleFrame(std::uint32_t frame, ThunderVector3 scale,
                            std::uint32_t combine) = 0;
    virtual void RotateFrame(std::uint32_t frame, ThunderVector3 axis,
                             float degrees, std::uint32_t combine) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                ThunderVector3 position,
                                std::uint32_t combine) = 0;
    [[nodiscard]] virtual std::uint32_t FrameUpdateObjects(
        std::uint32_t frame) = 0;
};

class ThunderSourceFrameRuntimeStub final
    : public IThunderSourceFrameRuntime {
public:
    void FrameSetIdentity(std::uint32_t) override {}
    void ScaleFrame(std::uint32_t, ThunderVector3,
                    std::uint32_t) override {}
    void RotateFrame(std::uint32_t, ThunderVector3, float,
                     std::uint32_t) override {}
    void TranslateFrame(std::uint32_t, ThunderVector3,
                        std::uint32_t) override {}
    [[nodiscard]] std::uint32_t FrameUpdateObjects(
        std::uint32_t) override { return 0u; }
};

}  // namespace pal4
