#include "pal4/grid_vector_math.h"

#include <cmath>

namespace pal4 {
namespace {

double Dot(const GridVector3& first, const GridVector3& second) {
    return static_cast<double>(first[0]) * second[0] +
           static_cast<double>(first[1]) * second[1] +
           static_cast<double>(first[2]) * second[2];
}

}  // namespace

GridVector3& GridVectorMath::ConstrainDirection(
    GridVector3& destination, const bool adjust_below_threshold,
    const GridVector3& velocity, const float threshold,
    const GridVector3& normal) {
    const double length = std::sqrt(Dot(velocity, velocity));
    if (length == 0.0) {
        destination = velocity;
        return destination;
    }
    const double cosine = Dot(velocity, normal) / length;
    const bool adjust = adjust_below_threshold ?
        cosine < threshold : cosine > threshold;
    if (!adjust) {
        destination = velocity;
        return destination;
    }
    const double parallel = Dot(velocity, normal);
    GridVector3 tangent{};
    for (unsigned axis = 0u; axis < 3u; ++axis)
        tangent[axis] = static_cast<float>(velocity[axis] -
                                           parallel * normal[axis]);
    const double tangent_length = std::sqrt(Dot(tangent, tangent));
    const double lateral = std::sqrt(1.0 -
                                     static_cast<double>(threshold) * threshold);
    for (unsigned axis = 0u; axis < 3u; ++axis) {
        const double unit_tangent = tangent_length == 0.0 ?
            tangent[axis] : tangent[axis] / tangent_length;
        destination[axis] = static_cast<float>(
            (unit_tangent * lateral +
             static_cast<double>(threshold) * normal[axis]) * length);
    }
    return destination;
}

}  // namespace pal4
