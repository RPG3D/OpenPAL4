#pragma once

#include <array>

#include <cstddef>

namespace pal4 {

struct FrameRotationState final {
    std::array<float, 19> words{};

    [[nodiscard]] float* Axis(std::size_t index) {
        return words.data() + 4 + index * 4;
    }
};

class IFrameRotationRuntime {
public:
    virtual ~IFrameRotationRuntime() = default;
    virtual void ClampAxis(float* axis) = 0;
    virtual void Translate(FrameRotationState& frame,
                           const std::array<float, 3>& offset,
                           unsigned mode) = 0;
    virtual void Rotate(FrameRotationState& frame, float* axis,
                        float degrees, unsigned mode) = 0;
    virtual void UpdateObjects(FrameRotationState& frame) = 0;
    [[nodiscard]] virtual int CalculateAngles(
        const float* axis, std::array<float, 3>& output) = 0;
};

class FrameRotation final {
public:
    [[nodiscard]] static int Update(
        FrameRotationState& frame, const std::array<float, 3>& angles,
        std::array<float, 3>& output, IFrameRotationRuntime& runtime);
};

}  // namespace pal4
