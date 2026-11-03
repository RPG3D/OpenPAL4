#include "pal4/object_movement_vectors.h"

#include <bit>

namespace pal4 {
namespace {

BehaviorVector3 Read(const ObjectMovementState& state, std::size_t offset) {
    return {std::bit_cast<float>(state.words[offset]),
            std::bit_cast<float>(state.words[offset + 1u]),
            std::bit_cast<float>(state.words[offset + 2u])};
}

void Write(ObjectMovementState& state, std::size_t offset,
           const BehaviorVector3& value) {
    for (std::size_t i = 0u; i != 3u; ++i)
        state.words[offset + i] = std::bit_cast<std::uint32_t>(value[i]);
}

}  // namespace

void ObjectMovementVectorRuntimeStub::ProcessDifference(
    ObjectMovementState&, BehaviorVector3&, const BehaviorVector3&) {}
BehaviorVector3 ObjectMovementVectorRuntimeStub::TransformVector(
    ObjectMovementState&, std::uint32_t) { return {}; }

void ObjectMovementVectors::IgnoreFourArguments(
    std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) {}

BehaviorVector3& ObjectMovementVectors::CopyAt4(
    const ObjectMovementState& state, BehaviorVector3& output) {
    output = Read(state, 1u);
    return output;
}

BehaviorVector3& ObjectMovementVectors::CopyAt16(
    const ObjectMovementState& state, BehaviorVector3& output) {
    output = Read(state, 4u);
    return output;
}

BehaviorVector3& ObjectMovementVectors::CopyAt28(
    const ObjectMovementState& state, BehaviorVector3& output) {
    output = Read(state, 7u);
    return output;
}

BehaviorVector3& ObjectMovementVectors::CopyAt40(
    const ObjectMovementState& state, BehaviorVector3& output) {
    output = Read(state, 10u);
    return output;
}

BehaviorVector3& ObjectMovementVectors::SetAt4(
    ObjectMovementState& state, BehaviorVector3& output,
    const BehaviorVector3& value) {
    Write(state, 1u, value);
    output = Read(state, 1u);
    return output;
}

BehaviorVector3& ObjectMovementVectors::SetAt40(
    ObjectMovementState& state, BehaviorVector3& output,
    const BehaviorVector3& value) {
    Write(state, 10u, value);
    output = Read(state, 10u);
    return output;
}

BehaviorVector3& ObjectMovementVectors::DifferenceAndProcess(
    ObjectMovementState& state, BehaviorVector3& output,
    const BehaviorVector3& target, IObjectMovementVectorRuntime& runtime) {
    const auto origin = Read(state, 10u);
    const BehaviorVector3 difference{target[0] - origin[0],
                                     target[1] - origin[1],
                                     target[2] - origin[2]};
    runtime.ProcessDifference(state, output, difference);
    return output;
}

BehaviorVector3& ObjectMovementVectors::TransformAndAdd(
    ObjectMovementState& state, BehaviorVector3& output,
    std::uint32_t input, IObjectMovementVectorRuntime& runtime) {
    const auto transformed = runtime.TransformVector(state, input);
    const auto origin = Read(state, 10u);
    output = {transformed[0] + origin[0], transformed[1] + origin[1],
              transformed[2] + origin[2]};
    return output;
}

}  // namespace pal4
