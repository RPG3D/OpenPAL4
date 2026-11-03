#pragma once

#include <cstdint>

#include "pal4/d3d_triangle_face.h"

namespace pal4 {

class D3dFloatArray final {
public:
    // IDA 0x602d30.
    [[nodiscard]] static std::uint32_t ElementAddress(
        std::uint32_t base_address, std::uint32_t index);
};

class D3dVector2Operations final {
public:
    // IDA 0x602d40/0x602d60/0x602d80/0x602da0/0x602dc0.
    static D3dTriangleVector2& Copy(D3dTriangleVector2& output,
                                    const D3dTriangleVector2& input);
    static D3dTriangleVector2& AddInPlace(
        D3dTriangleVector2& value, const D3dTriangleVector2& added);
    static D3dTriangleVector2& SubtractInPlace(
        D3dTriangleVector2& value, const D3dTriangleVector2& removed);
    static D3dTriangleVector2& ScaleInPlace(
        D3dTriangleVector2& value, float factor);
    [[nodiscard]] static bool NotEqual(
        const D3dTriangleVector2& first,
        const D3dTriangleVector2& second);
    // IDA 0x602df0/0x602e30/0x602e70.
    static D3dTriangleVector2& Add(
        D3dTriangleVector2& output,
        const D3dTriangleVector2& first,
        const D3dTriangleVector2& second);
    static D3dTriangleVector2& Subtract(
        D3dTriangleVector2& output,
        const D3dTriangleVector2& first,
        const D3dTriangleVector2& second);
    static D3dTriangleVector2& Multiply(
        D3dTriangleVector2& output,
        const D3dTriangleVector2& input, float factor);
    // IDA 0x602ea0/0x602ed0/0x602ef0/0x603150.
    static D3dTriangleVector2& MultiplyLeft(
        D3dTriangleVector2& output, float factor,
        const D3dTriangleVector2& input);
    [[nodiscard]] static double Length(const D3dTriangleVector2& value);
    static D3dTriangleVector2& NormalizeInPlace(D3dTriangleVector2& value);
    [[nodiscard]] static double Dot(const D3dTriangleVector2& first,
                                    const D3dTriangleVector2& second);
};

}  // namespace pal4
