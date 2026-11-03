#pragma once

#include <array>
#include <limits>

namespace pal4 {

class GameWorldIntersectionMath final {
public:
    using Plane = std::array<float, 6>;
    using Vector = std::array<float, 3>;
    static constexpr double kParallelThreshold = 0.000001;

    [[nodiscard]] static double CalculateParameter(
        const Plane& plane, const Vector& direction,
        const Vector& point);
};

}  // namespace pal4
