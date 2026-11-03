#include "pal4/d3d_vector2_operations.h"

#include <cmath>

namespace pal4 {

std::uint32_t D3dFloatArray::ElementAddress(
    const std::uint32_t base_address, const std::uint32_t index) {
    return base_address + 4u * index;
}

D3dTriangleVector2& D3dVector2Operations::Copy(
    D3dTriangleVector2& output, const D3dTriangleVector2& input) {
    output = input;
    return output;
}

D3dTriangleVector2& D3dVector2Operations::AddInPlace(
    D3dTriangleVector2& value, const D3dTriangleVector2& added) {
    value.x += added.x;
    value.z += added.z;
    return value;
}

D3dTriangleVector2& D3dVector2Operations::SubtractInPlace(
    D3dTriangleVector2& value, const D3dTriangleVector2& removed) {
    value.x -= removed.x;
    value.z -= removed.z;
    return value;
}

D3dTriangleVector2& D3dVector2Operations::ScaleInPlace(
    D3dTriangleVector2& value, const float factor) {
    value.x *= factor;
    value.z *= factor;
    return value;
}

bool D3dVector2Operations::NotEqual(
    const D3dTriangleVector2& first,
    const D3dTriangleVector2& second) {
    return first.x != second.x || first.z != second.z;
}

D3dTriangleVector2& D3dVector2Operations::Add(
    D3dTriangleVector2& output, const D3dTriangleVector2& first,
    const D3dTriangleVector2& second) {
    auto temporary = first;
    AddInPlace(temporary, second);
    return Copy(output, temporary);
}

D3dTriangleVector2& D3dVector2Operations::Subtract(
    D3dTriangleVector2& output, const D3dTriangleVector2& first,
    const D3dTriangleVector2& second) {
    auto temporary = first;
    SubtractInPlace(temporary, second);
    return Copy(output, temporary);
}

D3dTriangleVector2& D3dVector2Operations::Multiply(
    D3dTriangleVector2& output, const D3dTriangleVector2& input,
    const float factor) {
    const D3dTriangleVector2 result{factor * input.x,
                                     factor * input.z};
    return Copy(output, result);
}

D3dTriangleVector2& D3dVector2Operations::MultiplyLeft(
    D3dTriangleVector2& output, const float factor,
    const D3dTriangleVector2& input) {
    return Multiply(output, input, factor);
}

double D3dVector2Operations::Length(const D3dTriangleVector2& value) {
    return std::sqrt(static_cast<double>(value.x) * value.x +
                     static_cast<double>(value.z) * value.z);
}

D3dTriangleVector2& D3dVector2Operations::NormalizeInPlace(
    D3dTriangleVector2& value) {
    const auto length = Length(value);
    const auto inverse = length > 0.0 ? 1.0 / length : 0.0;
    value.x = static_cast<float>(inverse * value.x);
    value.z = static_cast<float>(inverse * value.z);
    return value;
}

double D3dVector2Operations::Dot(const D3dTriangleVector2& first,
                                 const D3dTriangleVector2& second) {
    // IDA60315B/603160/603162 run FMUL/FMUL/FADDP under game PC24.
    // Preserve each rounding boundary before widening the return value. At
    // M11 face908 an edge midpoint cancels to exactly zero; double products
    // instead classify it outside and spuriously clamp the path node inward.
    // MSVC x86 can keep ordinary float locals in extended x87 registers.
    // Volatile stores enforce PC24 boundaries even outside the game's FPU mode.
    const volatile float z_product = first.z * second.z;
    const volatile float x_product = first.x * second.x;
    const volatile float result = z_product + x_product;
    return result;
}

}  // namespace pal4
