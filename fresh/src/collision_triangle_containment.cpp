#include "pal4/collision_triangle_containment.h"

#include <cmath>
#include <cstddef>

namespace pal4 {

bool CollisionTriangleContainment::ContainsProjectedPoint(
    const CollisionPoint3& point,
    const std::array<CollisionPoint3, 3u>& vertices,
    const CollisionPoint3& normal) {
    const auto x = std::fabs(normal[0]);
    const auto y = std::fabs(normal[1]);
    const auto z = std::fabs(normal[2]);
    const std::size_t dominant_axis = z > y ? (z > x ? 2u : 0u) :
                                               (y > x ? 1u : 0u);
    const std::size_t vertical_axis = dominant_axis == 1u ? 2u : 1u;
    const std::size_t horizontal_axis = dominant_axis == 0u ? 2u : 0u;

    bool inside = false;
    std::size_t previous = 2u;
    for (std::size_t current = 0u; current < 3u; ++current) {
        const auto current_vertical = vertices[current][vertical_axis];
        const auto previous_vertical = vertices[previous][vertical_axis];
        const auto point_vertical = point[vertical_axis];
        if ((current_vertical <= point_vertical &&
             point_vertical < previous_vertical) ||
            (previous_vertical <= point_vertical &&
             point_vertical < current_vertical)) {
            const auto crossing =
                (static_cast<double>(vertices[previous][horizontal_axis]) -
                 vertices[current][horizontal_axis]) *
                    (static_cast<double>(point_vertical) - current_vertical) /
                    (static_cast<double>(previous_vertical) -
                     current_vertical) +
                vertices[current][horizontal_axis];
            if (crossing > point[horizontal_axis]) inside = !inside;
        }
        previous = current;
    }
    return inside;
}

bool CollisionTriangleContainment::IntersectLineWithTriangle(
    const CollisionPoint3& origin,
    const CollisionPoint3& direction,
    const std::array<CollisionPoint3, 3u>& vertices,
    CollisionPoint3* intersection) {
    const auto& a = vertices[0];
    const auto& b = vertices[1];
    const auto& c = vertices[2];
    const CollisionPoint3 edge_one{
        b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const CollisionPoint3 edge_two{
        c[0] - a[0], c[1] - a[1], c[2] - a[2]};
    const CollisionPoint3 origin_offset{
        origin[0] - a[0], origin[1] - a[1], origin[2] - a[2]};
    const CollisionPoint3 direction_cross{
        direction[1] * edge_two[2] - direction[2] * edge_two[1],
        direction[2] * edge_two[0] - edge_two[2] * direction[0],
        edge_two[1] * direction[0] - direction[1] * edge_two[0]};
    const auto determinant =
        static_cast<double>(direction_cross[1]) * edge_one[1] +
        static_cast<double>(direction_cross[0]) * edge_one[0] +
        static_cast<double>(direction_cross[2]) * edge_one[2];
    if (determinant == 0.0) return false;
    const auto reciprocal = 1.0 / determinant;
    const auto u = static_cast<float>(
        (static_cast<double>(direction_cross[1]) * origin_offset[1] +
         static_cast<double>(direction_cross[0]) * origin_offset[0] +
         static_cast<double>(direction_cross[2]) * origin_offset[2]) *
        reciprocal);
    const CollisionPoint3 offset_cross{
        origin_offset[1] * edge_one[2] - origin_offset[2] * edge_one[1],
        origin_offset[2] * edge_one[0] - origin_offset[0] * edge_one[2],
        origin_offset[0] * edge_one[1] - origin_offset[1] * edge_one[0]};
    const auto v = static_cast<float>(
        (static_cast<double>(direction[1]) * offset_cross[1] +
         static_cast<double>(offset_cross[0]) * direction[0] +
         static_cast<double>(direction[2]) * offset_cross[2]) *
        reciprocal);
    if (!(u >= 0.0f) || !(v >= 0.0f) || u + v > 1.0f) return false;
    if (intersection != nullptr) {
        const auto weight_a = 1.0f - u - v;
        (*intersection)[0] = u * b[0] + weight_a * a[0] + v * c[0];
        (*intersection)[1] = v * c[1] + u * b[1] + weight_a * a[1];
        (*intersection)[2] = weight_a * a[2] + v * c[2] + u * b[2];
    }
    return true;
}

}  // namespace pal4
