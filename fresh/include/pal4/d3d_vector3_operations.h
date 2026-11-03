#pragma once

#include "pal4/d3d_triangle_face.h"

namespace pal4 {

class D3dVector3Operations final {
public:
    // IDA 0x602f30/0x602f50/0x602f80/0x602fb0.
    static D3dTriangleVector3& Copy(D3dTriangleVector3& output,
                                    const D3dTriangleVector3& input);
    static D3dTriangleVector3& AddInPlace(
        D3dTriangleVector3& value, const D3dTriangleVector3& added);
    static D3dTriangleVector3& SubtractInPlace(
        D3dTriangleVector3& value, const D3dTriangleVector3& removed);
    static D3dTriangleVector3& ScaleInPlace(D3dTriangleVector3& value,
                                            float factor);

    // IDA 0x602fe0/0x603020/0x603060/0x6030a0.
    [[nodiscard]] static bool Equal(const D3dTriangleVector3& first,
                                    const D3dTriangleVector3& second);
    [[nodiscard]] static bool NotEqual(const D3dTriangleVector3& first,
                                       const D3dTriangleVector3& second);
    static D3dTriangleVector3& Add(D3dTriangleVector3& output,
                                   const D3dTriangleVector3& first,
                                   const D3dTriangleVector3& second);
    static D3dTriangleVector3& Subtract(D3dTriangleVector3& output,
                                        const D3dTriangleVector3& first,
                                        const D3dTriangleVector3& second);

    // IDA 0x6030e0/0x603110/0x603170/0x603190.
    [[nodiscard]] static double Length(const D3dTriangleVector3& value);
    static D3dTriangleVector3& NormalizeInPlace(D3dTriangleVector3& value);
    [[nodiscard]] static double Dot(const D3dTriangleVector3& first,
                                    const D3dTriangleVector3& second);
    static D3dTriangleVector3& Cross(D3dTriangleVector3& output,
                                     const D3dTriangleVector3& first,
                                     const D3dTriangleVector3& second);
};

}  // namespace pal4
