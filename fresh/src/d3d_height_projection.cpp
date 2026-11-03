#include "pal4/d3d_height_projection.h"

namespace pal4 {

std::uint32_t D3dHeightProjection::Project(
    D3dTriangleVector3& point, ID3dHeightRuntime& runtime) {
    const auto result = runtime.Evaluate(point.x, point.z);
    point.y = result.height;
    return result.eax;
}

}  // namespace pal4
