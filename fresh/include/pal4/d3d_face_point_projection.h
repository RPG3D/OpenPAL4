#pragma once

#include "pal4/d3d_height_projection.h"
#include "pal4/d3d_triangle_face.h"

namespace pal4 {

class D3dFacePointProjection final {
public:
    // IDA 0x6039c0.
    static D3dTriangleVector3& Project(
        D3dTriangleVector3& output,
        D3dTriangleFaceState& face,
        const D3dTriangleVector3& input,
        ID3dTriangleTraceRuntime& trace,
        ID3dHeightRuntime& height);
};

}  // namespace pal4
