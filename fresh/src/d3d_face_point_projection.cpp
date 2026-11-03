#include "pal4/d3d_face_point_projection.h"

#include "pal4/d3d_vector3_operations.h"

namespace pal4 {

D3dTriangleVector3& D3dFacePointProjection::Project(
    D3dTriangleVector3& output,
    D3dTriangleFaceState& face,
    const D3dTriangleVector3& input,
    ID3dTriangleTraceRuntime& trace,
    ID3dHeightRuntime& height) {
    D3dTriangleVector3 point{};
    D3dVector3Operations::Copy(point, input);
    if (!D3dTriangleFace::IsOutsideEdges3D(face, point, trace)) {
        static_cast<void>(D3dTriangleFace::ClampPointToFace(face, point,
                                                              trace));
    }
    static_cast<void>(D3dHeightProjection::Project(point, height));
    return D3dVector3Operations::Copy(output, point);
}

}  // namespace pal4
