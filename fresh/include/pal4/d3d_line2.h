#pragma once

#include "pal4/d3d_triangle_face.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

struct D3dLine2State final {
    D3dTriangleVector2 start;
    D3dTriangleVector2 end;
    D3dTriangleVector2 normal;
    std::uint8_t normal_cached = 0;
    std::array<std::uint8_t, 3> padding{};
};

static_assert(sizeof(D3dLine2State) == 28);
static_assert(offsetof(D3dLine2State, normal) == 16);
static_assert(offsetof(D3dLine2State, normal_cached) == 24);

class D3dLine2 final {
public:
    // IDA 0x6031e0/0x6031f0/0x603220/0x603240.
    static void ClearNormalCache(D3dLine2State& line);
    static D3dLine2State& SetEndpoints(D3dLine2State& line,
                                      const D3dTriangleVector2& start,
                                      const D3dTriangleVector2& end);
    static D3dTriangleVector2& SetStart(D3dLine2State& line,
                                        const D3dTriangleVector2& start);
    static D3dTriangleVector2& SetEnd(D3dLine2State& line,
                                      const D3dTriangleVector2& end);

    // IDA 0x603260/0x603280/0x6032b0/0x6032f0.
    static D3dTriangleVector2& Normal(D3dLine2State& line);
    static D3dTriangleVector2& Reinitialize(
        D3dLine2State& line, const D3dTriangleVector2& start,
        const D3dTriangleVector2& end);
    [[nodiscard]] static double SignedDistance(
        D3dLine2State& line, const D3dTriangleVector2& point);
    [[nodiscard]] static int Classify(D3dLine2State& line,
                                      const D3dTriangleVector2& point,
                                      float tolerance);

    // IDA 0x603340. The result distinguishes line and segment intersections.
    [[nodiscard]] static int Intersect(const D3dLine2State& first,
                                       const D3dLine2State& second,
                                       D3dTriangleVector2* intersection);

    // IDA 0x603520/0x603530/0x603560.
    static D3dTriangleVector2& End(D3dLine2State& line);
    static D3dTriangleVector2& ComputeUnitDirection(
        D3dLine2State& line, D3dTriangleVector2& output);
    static D3dTriangleVector2& ComputeNormal(D3dLine2State& line);
};

}  // namespace pal4
