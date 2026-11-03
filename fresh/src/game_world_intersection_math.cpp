#include "pal4/game_world_intersection_math.h"

#include <cmath>

namespace pal4 {

double GameWorldIntersectionMath::CalculateParameter(
    const Plane& plane, const Vector& direction, const Vector& point) {
    const double denominator =
        static_cast<double>(plane[4]) * direction[1] +
        static_cast<double>(plane[3]) * direction[0] +
        static_cast<double>(plane[5]) * direction[2];
    if (denominator < kParallelThreshold &&
        denominator > -kParallelThreshold)
        return std::numeric_limits<float>::max();

    const double numerator =
        static_cast<double>(direction[1]) *
            (static_cast<double>(point[1]) - plane[1]) +
        (static_cast<double>(point[0]) - plane[0]) * direction[0] +
        static_cast<double>(direction[2]) *
            (static_cast<double>(point[2]) - plane[2]);
    return numerator / denominator;
}

}  // namespace pal4
