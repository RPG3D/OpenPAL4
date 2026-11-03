#include "pal4/vector3_operations.h"

#include <bit>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {

using pal4::BehaviorVector3;
using pal4::Vector3Operations;

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Random final : pal4::IVectorRandomSource {
    std::int32_t next = 0;
    int calls = 0;
    std::int32_t Rand() override {
        ++calls;
        return next;
    }
};

void TestArithmetic() {
    BehaviorVector3 first{3.0f, 4.0f, 5.0f};
    const BehaviorVector3 second{1.0f, 2.0f, 3.0f};
    BehaviorVector3 output{};
    Check(&Vector3Operations::Add(first, output, second) == &output &&
          output == BehaviorVector3{4.0f, 6.0f, 8.0f}, "vector add");
    Check(&Vector3Operations::Subtract(first, output, second) == &output &&
          output == BehaviorVector3{2.0f, 2.0f, 2.0f}, "vector subtract");
    Check(&Vector3Operations::Scale(first, output, 2.0f) == &output &&
          output == BehaviorVector3{6.0f, 8.0f, 10.0f}, "vector scale");
    Check(&Vector3Operations::SetXYZBits(output, 0x3f800000u,
                                         0xbf800000u, 0x40000000u) == &output &&
          output == BehaviorVector3{1.0f, -1.0f, 2.0f}, "bit-exact vector set");
    Check(Vector3Operations::Equals(first, first) &&
          !Vector3Operations::Equals(first, second), "component equality");
}

void TestNormalizeAndProjection() {
    BehaviorVector3 output{};
    Vector3Operations::Normalize({3.0f, 4.0f, 0.0f}, output);
    Check(output == BehaviorVector3{0.6f, 0.8f, 0.0f}, "normalize nonzero");
    Vector3Operations::Normalize({}, output);
    Check(output == BehaviorVector3{}, "normalize zero");
    const float nan = std::numeric_limits<float>::quiet_NaN();
    Vector3Operations::Normalize({nan, 1.0f, 2.0f}, output);
    Check(std::isnan(output[0]) && output[1] == 1.0f && output[2] == 2.0f,
          "unordered magnitude copies input");
    Vector3Operations::SubtractProjection(
        {3.0f, 4.0f, 5.0f}, output, {1.0f, 0.0f, 0.0f});
    Check(output == BehaviorVector3{0.0f, 4.0f, 5.0f},
          "subtract projection onto unit axis");
    Vector3Operations::CopyXZTo2D({3.0f, 4.0f, 5.0f}, output);
    Check(output == BehaviorVector3{3.0f, 0.0f, 5.0f}, "XZ projection");
    Check(&Vector3Operations::NormalizeByLength(
              {2.0f, 4.0f, 6.0f}, output, 2.0f) == &output &&
          output == BehaviorVector3{1.0f, 2.0f, 3.0f},
          "normalize by supplied length");
}

void TestInPlaceAndAlias() {
    BehaviorVector3 value{1.0f, 2.0f, 3.0f};
    BehaviorVector3 output{};
    Check(&Vector3Operations::CopyThrough(
              value, output, {4.0f, 5.0f, 6.0f}) == &output &&
          value == output && output == BehaviorVector3{4.0f, 5.0f, 6.0f},
          "copy through intermediate");
    Check(&Vector3Operations::AddInPlaceAndCopy(
              value, output, {1.0f, 2.0f, 3.0f}) == &output &&
          value == output && output == BehaviorVector3{5.0f, 7.0f, 9.0f},
          "in-place add then copy");
    const float factor = 2.0f;
    Check(&Vector3Operations::ScaleInPlaceAndCopy(
              value, output, factor) == &output &&
          value == output && output == BehaviorVector3{10.0f, 14.0f, 18.0f},
          "in-place scale then copy");
    Vector3Operations::Add(value, value, value);
    Check(value == BehaviorVector3{20.0f, 28.0f, 36.0f},
          "aliasing operands and output");
}

void TestRandomClamp() {
    Random random;
    Check(Vector3Operations::RandomRangeClamped(
              0.0f, 2.0f, -1.0f, 1.0f, random) == -1.0f &&
          random.calls == 1, "lower clamp");
    random.next = 32767;
    Check(Vector3Operations::RandomRangeClamped(
              0.0f, 2.0f, -1.0f, 1.0f, random) == 1.0f &&
          random.calls == 2, "upper clamp");
    random.next = 16384;
    Check(std::abs(Vector3Operations::RandomRangeClamped(
              0.25f, 0.5f, -1.0f, 1.0f, random) - 0.25f) < 0.001f,
          "unclamped middle");
    pal4::VectorRandomSourceStub stub;
    Check(Vector3Operations::RandomRangeClamped(
              1.0f, 0.0f, 0.0f, 2.0f, stub) == 1.0f,
          "linkable random stub");
}

}  // namespace

int main() {
    TestArithmetic();
    TestNormalizeAndProjection();
    TestInPlaceAndAlias();
    TestRandomClamp();
}
