#pragma once

#include "pal4/d3d_triangle_face.h"

#include <cstddef>

namespace pal4 {

struct D3dPlane3State final {
    D3dTriangleVector3 normal;
    D3dTriangleVector3 anchor;
    float offset = 0.0f;
};

static_assert(sizeof(D3dPlane3State) == 28);
static_assert(offsetof(D3dPlane3State, anchor) == 12);
static_assert(offsetof(D3dPlane3State, offset) == 24);

class D3dPlane3 final {
public:
    // IDA 0x603590.
    [[nodiscard]] static double HeightAt(const D3dPlane3State& plane,
                                         float x, float z);
    // IDA 0x6035e0.
    static void Initialize(D3dPlane3State& plane,
                           const D3dTriangleVector3& first,
                           const D3dTriangleVector3& second,
                           const D3dTriangleVector3& third);
};

}  // namespace pal4
