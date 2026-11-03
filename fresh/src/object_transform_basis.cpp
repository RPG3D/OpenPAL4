#include "pal4/object_transform_basis.h"
#include "movement_vector_math.h"

#include <bit>
#include <cmath>

namespace pal4 {
namespace {

BehaviorVector3 ReadAxis(const ObjectMovementState& state, std::size_t word) {
    return {std::bit_cast<float>(state.words[word]),
            std::bit_cast<float>(state.words[word + 1u]),
            std::bit_cast<float>(state.words[word + 2u])};
}

void WriteAxis(ObjectMovementState& state, std::size_t word,
               const BehaviorVector3& axis) {
    for (std::size_t i = 0u; i != 3u; ++i)
        state.words[word + i] = std::bit_cast<std::uint32_t>(axis[i]);
}

BehaviorVector3 Cross(const BehaviorVector3& a, const BehaviorVector3& b) {
    return {a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0]};
}

BehaviorVector3 Normalize(const BehaviorVector3& value) {
    return movement_detail::Normalize(value);
}

}  // namespace

std::uint32_t ObjectTransformRuntimeStub::OrientationFlag(ObjectMovementState&) {
    return 0u;
}
void ObjectTransformRuntimeStub::PrepareAxis(ObjectMovementState&) {}
std::uint32_t ObjectTransformRuntimeStub::ProcessNormalized(
    ObjectMovementState&, const BehaviorVector3&) { return 0u; }
void ObjectTransformRuntimeStub::MakeDifference(
    ObjectMovementState&, BehaviorVector3&, const BehaviorVector3&) {}
void ObjectTransformRuntimeStub::TransformDifference(
    ObjectMovementState&, BehaviorVector3&, const BehaviorVector3&) {}
void ObjectTransformRuntimeStub::ProcessTransformed(
    ObjectMovementState&, BehaviorVector3&, const BehaviorVector3&) {}

BehaviorVector3& ObjectTransformBasis::Multiply(
    const ObjectMovementState& state, BehaviorVector3& output,
    const BehaviorVector3& vector) {
    const auto first = ReadAxis(state, 1u);
    const auto second = ReadAxis(state, 4u);
    const auto third = ReadAxis(state, 7u);
    output = {vector[0] * first[0] + vector[1] * second[0] +
                  vector[2] * third[0],
              vector[0] * first[1] + vector[1] * second[1] +
                  vector[2] * third[1],
              vector[0] * first[2] + vector[1] * second[2] +
                  vector[2] * third[2]};
    return output;
}

std::uint32_t ObjectTransformBasis::NormalizeFirstAxis(
    ObjectMovementState& state, IObjectTransformRuntime& runtime) {
    const bool reversed = static_cast<std::uint8_t>(
        runtime.OrientationFlag(state)) != 0u;
    const auto second = ReadAxis(state, 4u);
    const auto third = ReadAxis(state, 7u);
    const auto first = Cross(reversed ? third : second,
                             reversed ? second : third);
    WriteAxis(state, 1u, first);
    const auto length=movement_detail::Length(first);
    if (!(length > 0.0)) return state.words[3];
    WriteAxis(state, 1u, Normalize(first));
    return 0u;
}

std::uint32_t ObjectTransformBasis::SetThirdAxis(
    ObjectMovementState& state, const BehaviorVector3& vector,
    IObjectTransformRuntime& runtime) {
    WriteAxis(state, 7u, vector);
    runtime.PrepareAxis(state);
    const auto result = runtime.OrientationFlag(state);
    const auto first = ReadAxis(state, 1u);
    const auto third = ReadAxis(state, 7u);
    const bool reversed = static_cast<std::uint8_t>(result) != 0u;
    WriteAxis(state, 4u, Cross(reversed ? first : third,
                               reversed ? third : first));
    return result;
}

std::uint32_t ObjectTransformBasis::NormalizeAndProcess(
    ObjectMovementState& state, const BehaviorVector3& vector,
    IObjectTransformRuntime& runtime) {
    // 42B440 inlines X,Y,Z, unlike Matrix_Normalize42B300's call to
    // CalculateDotProduct40BF20. Swap only the arithmetic input/output axes
    // to reuse that PC24 implementation while preserving this association.
    const auto normalized=Normalize({vector[2],vector[1],vector[0]});
    return runtime.ProcessNormalized(state,
        {normalized[2],normalized[1],normalized[0]});
}

BehaviorVector3& ObjectTransformBasis::ReorderAndNegate(
    ObjectMovementState& state, BehaviorVector3& output,
    const BehaviorVector3& vector, IObjectOrientationFlagRuntime& runtime) {
    const bool reversed = static_cast<std::uint8_t>(
        runtime.OrientationFlag(state)) != 0u;
    output = {reversed ? -vector[2] : vector[2], vector[1], vector[0]};
    return output;
}

BehaviorVector3& ObjectTransformBasis::ProcessTransformations(
    ObjectMovementState& state, BehaviorVector3& output,
    const BehaviorVector3& input, IObjectTransformRuntime& runtime) {
    BehaviorVector3 difference{};
    BehaviorVector3 transformed{};
    runtime.MakeDifference(state, difference, input);
    runtime.TransformDifference(state, transformed, difference);
    runtime.ProcessTransformed(state, output, transformed);
    return output;
}

}  // namespace pal4
