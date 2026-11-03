#include "pal4/d3d_vector2_operations.h"

#include <cstdlib>
#include <bit>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <limits>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

}  // namespace

int main() {
    using pal4::D3dTriangleVector2;
    using pal4::D3dVector2Operations;

    Check(pal4::D3dFloatArray::ElementAddress(0x1000u, 3u) == 0x100cu &&
              pal4::D3dFloatArray::ElementAddress(0xfffffffcu, 2u) == 4u,
          "float-element address uses 32-bit four-byte stride");

    D3dTriangleVector2 first{1.0f, -2.0f};
    const D3dTriangleVector2 second{3.0f, 4.0f};
    D3dTriangleVector2 output{};
    Check(&D3dVector2Operations::Copy(output, first) == &output &&
              output.x == 1.0f && output.z == -2.0f,
          "copy transfers both words and returns destination");
    Check(&D3dVector2Operations::AddInPlace(output, second) == &output &&
              output.x == 4.0f && output.z == 2.0f,
          "in-place add updates both components");
    Check(&D3dVector2Operations::SubtractInPlace(output, second) == &output &&
              output.x == 1.0f && output.z == -2.0f,
          "in-place subtract restores both components");
    Check(&D3dVector2Operations::ScaleInPlace(output, -2.0f) == &output &&
              output.x == -2.0f && output.z == 4.0f,
          "in-place scale multiplies both components");
    Check(D3dVector2Operations::NotEqual(first, second) &&
              !D3dVector2Operations::NotEqual(first, first),
          "comparison checks either component");
    const D3dTriangleVector2 opposite_zero{-0.0f, 0.0f};
    const D3dTriangleVector2 positive_zero{0.0f, -0.0f};
    Check(!D3dVector2Operations::NotEqual(opposite_zero, positive_zero),
          "signed zero compares equal");
    const D3dTriangleVector2 nan_value{
        std::numeric_limits<float>::quiet_NaN(), 0.0f};
    Check(D3dVector2Operations::NotEqual(nan_value, nan_value),
          "unordered first component compares unequal");

    Check(&D3dVector2Operations::Add(output, first, second) == &output &&
              output.x == 4.0f && output.z == 2.0f,
          "add wrapper returns the two-component sum");
    Check(&D3dVector2Operations::Subtract(output, first, second) == &output &&
              output.x == -2.0f && output.z == -6.0f,
          "subtract wrapper returns first minus second");
    Check(&D3dVector2Operations::Multiply(output, first, -3.0f) == &output &&
              output.x == -3.0f && output.z == 6.0f,
          "multiply wrapper scales both components");
    Check(&D3dVector2Operations::Add(first, first, second) == &first &&
              first.x == 4.0f && first.z == 2.0f,
          "wrapper copies through a temporary when output aliases input");
    Check(&D3dVector2Operations::SubtractInPlace(first, first) == &first &&
              first.x == 0.0f && first.z == 0.0f,
          "self-subtraction clears both components");

    Check(&D3dVector2Operations::MultiplyLeft(
              output, 2.0f, second) == &output &&
              output.x == 6.0f && output.z == 8.0f,
          "left scalar multiplication forwards both components");
    D3dTriangleVector2 diagonal{3.0f, 4.0f};
    Check(D3dVector2Operations::Length(diagonal) == 5.0,
          "two-component magnitude uses both squared coordinates");
    Check(D3dVector2Operations::Dot({2.0f, -3.0f},
                                    {4.0f, 5.0f}) == -7.0,
          "two-component dot product uses both coordinates");
    // Captured pair132 M11 face908 midpoint offset and its actual cached
    // edge1 normal. Each original PC24 product rounds to +/-35.6279411315918;
    // widened products instead sum to -5.517649697139859e-7.
    const D3dTriangleVector2 midpoint_offset{
        std::bit_cast<float>(0xc21a89c0u), std::bit_cast<float>(0xc2b83c80u)};
    const D3dTriangleVector2 edge_normal{
        std::bit_cast<float>(0xbf6c13ebu), std::bit_cast<float>(0x3ec605ddu)};
    Check(D3dVector2Operations::Dot(midpoint_offset, edge_normal) == 0.0 &&
              !std::signbit(D3dVector2Operations::Dot(midpoint_offset, edge_normal)),
          "captured M11 midpoint dot cancels exactly with original per-operation rounding");
    Check(&D3dVector2Operations::NormalizeInPlace(diagonal) == &diagonal &&
              std::fabs(diagonal.x - 0.6f) < 0.000001f &&
              std::fabs(diagonal.z - 0.8f) < 0.000001f,
          "normalization scales a nonzero vector to unit length");
    D3dTriangleVector2 zero{0.0f, -0.0f};
    Check(&D3dVector2Operations::NormalizeInPlace(zero) == &zero &&
              zero.x == 0.0f && zero.z == 0.0f &&
              std::signbit(zero.z),
          "zero-vector path multiplies by zero and preserves negative zero");
    auto not_finite = nan_value;
    D3dVector2Operations::NormalizeInPlace(not_finite);
    Check(std::isnan(not_finite.x),
          "unordered magnitude follows zero-factor path without hiding NaN");
}
