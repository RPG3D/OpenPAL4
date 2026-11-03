#pragma once

namespace pal4 {

class IFloatInterpolationRuntime {
public:
    virtual ~IFloatInterpolationRuntime() = default;
    [[nodiscard]] virtual double CosineApproximation(float radians) = 0;
};

class FloatInterpolator final {
public:
    [[nodiscard]] static double Interpolate(
        float start, float end, float fraction, int mode,
        IFloatInterpolationRuntime& runtime);
};

}  // namespace pal4
