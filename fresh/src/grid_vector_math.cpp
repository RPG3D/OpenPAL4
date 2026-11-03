#include "pal4/grid_vector_math.h"
#include "movement_vector_math.h"

#include <cmath>

namespace pal4 {
GridVector3& GridVectorMath::ConstrainDirection(
    GridVector3& destination, const bool adjust_below_threshold,
    const GridVector3& velocity, const float threshold,
    const GridVector3& normal) {
    const float length=movement_detail::Length(velocity);
    if (length == 0.f) {
        destination = velocity;
        return destination;
    }
    // 65EDB4..65EDE6 normalizes each component before the dot product.
    const volatile float inverse_length=1.f/length;
    const GridVector3 unit{velocity[0]*inverse_length,
                          velocity[1]*inverse_length,velocity[2]*inverse_length};
    const float cosine=movement_detail::Dot(unit,normal);
    const bool adjust = adjust_below_threshold ?
        cosine < threshold : cosine > threshold;
    if (!adjust) {
        destination = velocity;
        return destination;
    }
    const float parallel=movement_detail::Dot(velocity,normal);
    GridVector3 tangent{};
    for (unsigned axis = 0u; axis < 3u; ++axis)
    {
        const volatile float projected=parallel*normal[axis];
        tangent[axis]=velocity[axis]-projected;
    }
    const auto unit_tangent=movement_detail::Normalize(tangent);
    const volatile float threshold_squared=threshold*threshold;
    const volatile float lateral_squared=1.f-threshold_squared;
    const float lateral=static_cast<float>(std::sqrt(static_cast<double>(lateral_squared)));
    for (unsigned axis = 0u; axis < 3u; ++axis) {
        const volatile float tangent_part=unit_tangent[axis]*lateral;
        const volatile float normal_part=threshold*normal[axis];
        const volatile float combined=tangent_part+normal_part;
        destination[axis]=combined*length;
    }
    return destination;
}

}  // namespace pal4
