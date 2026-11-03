#include "pal4/d3d_vector3_operations.h"

#include <bit>
#include <cmath>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

using pal4::D3dTriangleVector3;
using pal4::D3dVector3Operations;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

bool ComponentsEqual(const D3dTriangleVector3& value, float x, float y,
                     float z) {
    return value.x == x && value.y == y && value.z == z;
}

void TestCopyAndInPlace() {
    const D3dTriangleVector3 source{1.0f, -2.0f, 3.0f};
    D3dTriangleVector3 value{};
    Check(&D3dVector3Operations::Copy(value, source) == &value &&
              ComponentsEqual(value, 1.0f, -2.0f, 3.0f),
          "copy all three words and return destination");
    const D3dTriangleVector3 signed_zero{-0.0f, 0.0f, -0.0f};
    D3dVector3Operations::Copy(value, signed_zero);
    Check(std::bit_cast<unsigned>(value.x) ==
              std::bit_cast<unsigned>(signed_zero.x) &&
              std::bit_cast<unsigned>(value.z) ==
              std::bit_cast<unsigned>(signed_zero.z),
          "copy preserves component bit patterns");
    D3dVector3Operations::Copy(value, source);
    Check(&D3dVector3Operations::AddInPlace(
               value, {4.0f, 5.0f, -6.0f}) == &value &&
              ComponentsEqual(value, 5.0f, 3.0f, -3.0f),
          "in-place add all three components");
    Check(&D3dVector3Operations::SubtractInPlace(
               value, {4.0f, 5.0f, -6.0f}) == &value &&
              ComponentsEqual(value, 1.0f, -2.0f, 3.0f),
          "in-place subtract all three components");
    Check(&D3dVector3Operations::ScaleInPlace(value, -2.0f) == &value &&
              ComponentsEqual(value, -2.0f, 4.0f, -6.0f),
          "in-place scalar multiply");
    D3dVector3Operations::AddInPlace(value, value);
    Check(ComponentsEqual(value, -4.0f, 8.0f, -12.0f),
          "in-place add supports self aliasing");
    D3dVector3Operations::SubtractInPlace(value, value);
    Check(ComponentsEqual(value, 0.0f, 0.0f, 0.0f),
          "in-place subtract supports self aliasing");
}

void TestComparison() {
    const D3dTriangleVector3 zero{};
    const D3dTriangleVector3 negative_zero{-0.0f, 0.0f, -0.0f};
    Check(D3dVector3Operations::Equal(zero, negative_zero) &&
              !D3dVector3Operations::NotEqual(zero, negative_zero),
          "signed zeros compare equal");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    for (const D3dTriangleVector3 candidate : {
             D3dTriangleVector3{nan, 0.0f, 0.0f},
             D3dTriangleVector3{0.0f, nan, 0.0f},
             D3dTriangleVector3{0.0f, 0.0f, nan}}) {
        Check(!D3dVector3Operations::Equal(candidate, candidate) &&
                  D3dVector3Operations::NotEqual(candidate, candidate),
              "unordered component makes vectors unequal");
    }
    Check(!D3dVector3Operations::Equal(zero, {0.0f, 0.0f, 1.0f}) &&
              D3dVector3Operations::NotEqual(zero, {0.0f, 0.0f, 1.0f}),
          "third component participates in comparison");
}

void TestResultConstruction() {
    const D3dTriangleVector3 first{1.0f, 2.0f, 3.0f};
    const D3dTriangleVector3 second{4.0f, -5.0f, 6.0f};
    D3dTriangleVector3 output{};
    Check(&D3dVector3Operations::Add(output, first, second) == &output &&
              ComponentsEqual(output, 5.0f, -3.0f, 9.0f),
          "add constructs output from a temporary");
    Check(&D3dVector3Operations::Subtract(output, first, second) == &output &&
              ComponentsEqual(output, -3.0f, 7.0f, -3.0f),
          "subtract preserves operand order");
    D3dVector3Operations::Add(output, output, second);
    Check(ComponentsEqual(output, 1.0f, 2.0f, 3.0f),
          "add permits output aliasing first input");
    D3dVector3Operations::Subtract(output, second, output);
    Check(ComponentsEqual(output, 3.0f, -7.0f, 3.0f),
          "subtract permits output aliasing second input");
}

void TestGeometry() {
    const D3dTriangleVector3 first{1.0f, 2.0f, 3.0f};
    const D3dTriangleVector3 second{4.0f, -5.0f, 6.0f};
    Check(D3dVector3Operations::Length({2.0f, 3.0f, 6.0f}) == 7.0,
          "length includes all three axes");
    Check(D3dVector3Operations::Dot(first, second) == 12.0,
          "dot product includes all three axes");
    D3dTriangleVector3 output{};
    Check(&D3dVector3Operations::Cross(output, first, second) == &output &&
              ComponentsEqual(output, 27.0f, 6.0f, -13.0f),
          "cross product orientation and return destination");
    D3dVector3Operations::Cross(output, output, first);
    Check(ComponentsEqual(output, 44.0f, -94.0f, 48.0f),
          "cross product supports output aliasing first input");

    D3dTriangleVector3 diagonal{0.0f, 3.0f, 4.0f};
    Check(&D3dVector3Operations::NormalizeInPlace(diagonal) == &diagonal &&
              std::fabs(diagonal.y - 0.6f) < 0.000001f &&
              std::fabs(diagonal.z - 0.8f) < 0.000001f,
          "normalize nonzero vector");
    D3dTriangleVector3 zero{0.0f, -0.0f, 0.0f};
    D3dVector3Operations::NormalizeInPlace(zero);
    Check(ComponentsEqual(zero, 0.0f, 0.0f, 0.0f) &&
              std::signbit(zero.y),
          "zero normalize preserves negative zero");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    D3dTriangleVector3 unordered{nan, 1.0f, 2.0f};
    D3dVector3Operations::NormalizeInPlace(unordered);
    Check(std::isnan(unordered.x) && unordered.y == 0.0f &&
              unordered.z == 0.0f,
          "unordered length follows zero-multiplier path");
}

}  // namespace

int main() {
    TestCopyAndInPlace();
    TestComparison();
    TestResultConstruction();
    // Original QN05 pair112 first new fork, equal target and incoming position.
    D3dTriangleVector3 delta{std::bit_cast<float>(1120347410u),std::bit_cast<float>(1059998144u),std::bit_cast<float>(1111450767u)};
    D3dVector3Operations::SubtractInPlace(delta,{std::bit_cast<float>(1115923986u),std::bit_cast<float>(1059998144u),std::bit_cast<float>(1087054483u)});
    D3dVector3Operations::NormalizeInPlace(delta);
    D3dVector3Operations::ScaleInPlace(delta,.017f*45.0f);
    Check(std::bit_cast<unsigned>(delta.x)==0x3ef71e72u && std::bit_cast<unsigned>(delta.z)==0x3f17f128u,
          "original QN05 displacement retains PC24 length and reciprocal boundaries");
    TestGeometry();
}
