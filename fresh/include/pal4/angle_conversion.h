#pragma once

#include <array>

namespace pal4 {

class IAngleConversionRuntime {
public:
    virtual ~IAngleConversionRuntime() = default;
    [[nodiscard]] virtual double AngleFromCosine(float cosine) = 0;
};

class AngleConversion final {
public:
    // IDA6629A0: arccos of the +Z dot, reflected for the negative-X hemisphere.
    [[nodiscard]] static double ConvertToDegrees(
        const std::array<float, 3>& direction,
        IAngleConversionRuntime& runtime);
};

}  // namespace pal4
