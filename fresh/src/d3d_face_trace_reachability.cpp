#include "pal4/d3d_face_trace_reachability.h"

namespace pal4 {

bool D3dFaceTraceReachability::CanReach(
    std::uint32_t face, const D3dTriangleVector3& start,
    const D3dTriangleVector3& end,
    ID3dFaceTraceReachabilityRuntime& runtime) {
    D3dLine2State line{};
    D3dLine2::SetEndpoints(line, {start.x, start.z},
                           {end.x, end.z});
    while (true) {
        std::uint32_t neighbor = 0u;
        const auto status = runtime.Trace(face, line, neighbor);
        if (status != 2u) return status == 1u;
        if (neighbor == 0u) return false;
        face = neighbor;
    }
}

}  // namespace pal4
