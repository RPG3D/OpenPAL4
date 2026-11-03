#include "pal4/collision_plane.h"

namespace pal4 {

double CollisionPlane::FromPoints(
    const CollisionPoint3& first, const CollisionPoint3& second,
    const CollisionPoint3& third, CollisionPoint3* normal,
    ICollisionPlaneRuntime& runtime) {
    if (normal == nullptr) return 0.0;
    const auto edge_x = static_cast<double>(second[0]) - first[0];
    const auto edge_y = static_cast<double>(second[1]) - first[1];
    const auto edge_z = static_cast<double>(second[2]) - first[2];
    const auto back_x = first[0] - third[0];
    const auto back_y = first[1] - third[1];
    const auto back_z = static_cast<double>(first[2]) - third[2];
    (*normal)[0] = static_cast<float>(back_z * edge_y - back_y * edge_z);
    (*normal)[1] = static_cast<float>(back_x * edge_z - back_z * edge_x);
    (*normal)[2] = static_cast<float>(back_y * edge_x - back_x * edge_y);
    runtime.Normalize(*normal);
    return -(static_cast<double>(first[1]) * (*normal)[1] +
             static_cast<double>(first[0]) * (*normal)[0] +
             static_cast<double>((*normal)[2]) * first[2]);
}

}  // namespace pal4
