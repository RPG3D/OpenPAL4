#include "pal4/d3d_plane3.h"

#include "pal4/d3d_vector3_operations.h"

namespace pal4 {

double D3dPlane3::HeightAt(const D3dPlane3State& plane,
                           const float x, const float z) {
    if (plane.normal.y == 0.0f) return 0.0;
    // IDA6035B3..6035C9: x87 PC24 rounds each multiply/add/divide.
    // Widening the expression changes sloped-floor heights before storage.
    const float x_term = x * plane.normal.x;
    const float z_term = z * plane.normal.z;
    const float horizontal = x_term + z_term;
    const float numerator = horizontal + plane.offset;
    const float height = numerator / plane.normal.y;
    return -height;
}

void D3dPlane3::Initialize(D3dPlane3State& plane,
                           const D3dTriangleVector3& first,
                           const D3dTriangleVector3& second,
                           const D3dTriangleVector3& third) {
    D3dTriangleVector3 first_edge{};
    D3dTriangleVector3 second_edge{};
    D3dTriangleVector3 cross{};
    D3dVector3Operations::Subtract(first_edge, third, first);
    D3dVector3Operations::Subtract(second_edge, second, first);
    D3dVector3Operations::Cross(cross, second_edge, first_edge);
    D3dVector3Operations::Copy(plane.normal, cross);
    D3dVector3Operations::NormalizeInPlace(plane.normal);
    D3dVector3Operations::Copy(plane.anchor, first);
    plane.offset = static_cast<float>(
        -D3dVector3Operations::Dot(plane.anchor, plane.normal));
}

}  // namespace pal4
