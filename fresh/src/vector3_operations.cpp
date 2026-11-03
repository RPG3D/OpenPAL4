#include "pal4/vector3_operations.h"

#include <bit>
#include <cmath>

namespace pal4 {

std::int32_t VectorRandomSourceStub::Rand() { return 0; }

BehaviorVector3& Vector3Operations::SetXYZBits(
    BehaviorVector3& output, std::uint32_t x,
    std::uint32_t y, std::uint32_t z) {
    output = {std::bit_cast<float>(x), std::bit_cast<float>(y),
              std::bit_cast<float>(z)};
    return output;
}

BehaviorVector3& Vector3Operations::Add(
    const BehaviorVector3& first, BehaviorVector3& output,
    const BehaviorVector3& second) {
    const float z = first[2] + second[2];
    const float y = first[1] + second[1];
    const float x = first[0] + second[0];
    output = {x, y, z};
    return output;
}

BehaviorVector3& Vector3Operations::Subtract(
    const BehaviorVector3& first, BehaviorVector3& output,
    const BehaviorVector3& second) {
    const float z = first[2] - second[2];
    const float y = first[1] - second[1];
    const float x = first[0] - second[0];
    output = {x, y, z};
    return output;
}

BehaviorVector3& Vector3Operations::Scale(
    const BehaviorVector3& input, BehaviorVector3& output, float factor) {
    const float z = factor * input[2];
    const float y = factor * input[1];
    const float x = factor * input[0];
    output = {x, y, z};
    return output;
}

BehaviorVector3& Vector3Operations::Normalize(
    const BehaviorVector3& input, BehaviorVector3& output) {
    const double magnitude = std::sqrt(
        static_cast<double>(input[0]) * input[0] +
        static_cast<double>(input[1]) * input[1] +
        static_cast<double>(input[2]) * input[2]);
    if (!(magnitude > 0.0)) {
        output = input;
    } else {
        const float inverse = static_cast<float>(1.0 / magnitude);
        output = {inverse * input[0], inverse * input[1],
                  inverse * input[2]};
    }
    return output;
}

BehaviorVector3& Vector3Operations::NormalizeByLength(
    const BehaviorVector3& input, BehaviorVector3& output, float length) {
    const float inverse = 1.0f / length;
    output = {inverse * input[0], inverse * input[1],
              inverse * input[2]};
    return output;
}

BehaviorVector3& Vector3Operations::CopyThrough(
    BehaviorVector3& intermediate, BehaviorVector3& output,
    const BehaviorVector3& input) {
    intermediate[0] = input[0];
    intermediate[1] = input[1];
    intermediate[2] = input[2];
    output[0] = intermediate[0];
    output[1] = intermediate[1];
    output[2] = intermediate[2];
    return output;
}

BehaviorVector3& Vector3Operations::AddInPlaceAndCopy(
    BehaviorVector3& value, BehaviorVector3& output,
    const BehaviorVector3& added) {
    const float z = added[2] + value[2];
    const float y = added[1] + value[1];
    value[0] = added[0] + value[0];
    value[2] = z;
    value[1] = y;
    output = value;
    return output;
}

BehaviorVector3& Vector3Operations::ScaleInPlaceAndCopy(
    BehaviorVector3& value, BehaviorVector3& output,
    const float& factor) {
    const float y = factor * value[1];
    const float z = factor * value[2];
    value[0] = factor * value[0];
    value[2] = z;
    value[1] = y;
    output = value;
    return output;
}

bool Vector3Operations::Equals(const BehaviorVector3& first,
                               const BehaviorVector3& second) {
    return first[0] == second[0] && first[1] == second[1] &&
           first[2] == second[2];
}

BehaviorVector3& Vector3Operations::SubtractProjection(
    const BehaviorVector3& value, BehaviorVector3& output,
    const BehaviorVector3& axis) {
    const double dot = static_cast<double>(axis[0]) * value[0] +
                       static_cast<double>(value[2]) * axis[2] +
                       static_cast<double>(value[1]) * axis[1];
    const float x = static_cast<float>(value[0] - dot * axis[0]);
    const float y = static_cast<float>(value[1] - dot * axis[1]);
    const float z = static_cast<float>(value[2] - dot * axis[2]);
    output = {x, y, z};
    return output;
}

BehaviorVector3& Vector3Operations::CopyXZTo2D(
    const BehaviorVector3& input, BehaviorVector3& output) {
    output = {input[0], 0.0f, input[2]};
    return output;
}

float Vector3Operations::RandomRangeClamped(
    float center, float radius, float minimum, float maximum,
    IVectorRandomSource& random) {
    constexpr float kRandScale = 0.000030518509f;
    const double sample = static_cast<double>(random.Rand()) * kRandScale;
    const double value = (sample + sample - 1.0) * radius + center;
    if (value < minimum) return minimum;
    if (value > maximum) return maximum;
    return static_cast<float>(value);
}

}  // namespace pal4
