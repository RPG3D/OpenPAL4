#include "pal4/d3d_vector3_operations.h"

#include <cmath>

namespace pal4 {

D3dTriangleVector3& D3dVector3Operations::Copy(
    D3dTriangleVector3& output, const D3dTriangleVector3& input) {
    output = input;
    return output;
}

D3dTriangleVector3& D3dVector3Operations::AddInPlace(
    D3dTriangleVector3& value, const D3dTriangleVector3& added) {
    value.x += added.x;
    value.y += added.y;
    value.z += added.z;
    return value;
}

D3dTriangleVector3& D3dVector3Operations::SubtractInPlace(
    D3dTriangleVector3& value, const D3dTriangleVector3& removed) {
    value.x -= removed.x;
    value.y -= removed.y;
    value.z -= removed.z;
    return value;
}

D3dTriangleVector3& D3dVector3Operations::ScaleInPlace(
    D3dTriangleVector3& value, const float factor) {
    value.x *= factor;
    value.y *= factor;
    value.z *= factor;
    return value;
}

bool D3dVector3Operations::Equal(const D3dTriangleVector3& first,
                                 const D3dTriangleVector3& second) {
    return first.x == second.x && first.y == second.y &&
           first.z == second.z;
}

bool D3dVector3Operations::NotEqual(const D3dTriangleVector3& first,
                                    const D3dTriangleVector3& second) {
    return first.x != second.x || first.y != second.y ||
           first.z != second.z;
}

D3dTriangleVector3& D3dVector3Operations::Add(
    D3dTriangleVector3& output, const D3dTriangleVector3& first,
    const D3dTriangleVector3& second) {
    auto temporary = first;
    AddInPlace(temporary, second);
    return Copy(output, temporary);
}

D3dTriangleVector3& D3dVector3Operations::Subtract(
    D3dTriangleVector3& output, const D3dTriangleVector3& first,
    const D3dTriangleVector3& second) {
    auto temporary = first;
    SubtractInPlace(temporary, second);
    return Copy(output, temporary);
}

double D3dVector3Operations::Length(const D3dTriangleVector3& value) {
    // IDA6030EA..6030F8: each product/sum and FSQRT uses x87 PC24.
    const float xx=value.x*value.x;
    const float yy=value.y*value.y;
    const float zz=value.z*value.z;
    const float xy=xx+yy;
    const float squared=xy+zz;
    return static_cast<float>(std::sqrt(static_cast<double>(squared)));
}

D3dTriangleVector3& D3dVector3Operations::NormalizeInPlace(
    D3dTriangleVector3& value) {
    const auto length = Length(value);
    // IDA603124 rounds the reciprocal before multiplying each component.
    const float inverse = length > 0.0 ? static_cast<float>(1.0 / length) : 0.0f;
    value.x = static_cast<float>(inverse * value.x);
    value.y = static_cast<float>(inverse * value.y);
    value.z = static_cast<float>(inverse * value.z);
    return value;
}

double D3dVector3Operations::Dot(const D3dTriangleVector3& first,
                                 const D3dTriangleVector3& second) {
    return static_cast<double>(first.z) * second.z +
           static_cast<double>(first.y) * second.y +
           static_cast<double>(first.x) * second.x;
}

D3dTriangleVector3& D3dVector3Operations::Cross(
    D3dTriangleVector3& output, const D3dTriangleVector3& first,
    const D3dTriangleVector3& second) {
    const D3dTriangleVector3 result{
        static_cast<float>(static_cast<double>(first.y) * second.z -
                           static_cast<double>(first.z) * second.y),
        static_cast<float>(static_cast<double>(first.z) * second.x -
                           static_cast<double>(first.x) * second.z),
        static_cast<float>(static_cast<double>(first.x) * second.y -
                           static_cast<double>(first.y) * second.x)};
    return Copy(output, result);
}

}  // namespace pal4
