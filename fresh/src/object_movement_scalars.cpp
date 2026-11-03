#include "pal4/object_movement_scalars.h"

#include <bit>

namespace pal4 {
namespace {

float Read(const ObjectMovementState& state, std::size_t word) {
    return std::bit_cast<float>(state.words[word]);
}

float Write(ObjectMovementState& state, std::size_t word, float value) {
    state.words[word] = std::bit_cast<std::uint32_t>(value);
    return value;
}

}  // namespace

BehaviorVector3 ObjectMovementScalarRuntimeStub::ReadSlot10Vector(
    ObjectMovementState&) { return {}; }

float ObjectMovementScalars::Get23(const ObjectMovementState& state) {
    return Read(state, 23u);
}
float ObjectMovementScalars::Set23(ObjectMovementState& state, float value) {
    return Write(state, 23u, value);
}
BehaviorVector3& ObjectMovementScalars::ScaledSlot10Vector(
    ObjectMovementState& state, BehaviorVector3& output,
    IObjectMovementScalarRuntime& runtime) {
    const float scale = Read(state, 25u);
    const auto vector = runtime.ReadSlot10Vector(state);
    output = {scale * vector[0], scale * vector[1], scale * vector[2]};
    return output;
}
float ObjectMovementScalars::Get25(const ObjectMovementState& state) {
    return Read(state, 25u);
}
float ObjectMovementScalars::Set25(ObjectMovementState& state, float value) {
    return Write(state, 25u, value);
}
float ObjectMovementScalars::Get24(const ObjectMovementState& state) {
    return Read(state, 24u);
}
float ObjectMovementScalars::Set24(ObjectMovementState& state, float value) {
    return Write(state, 24u, value);
}
float ObjectMovementScalars::Get26(const ObjectMovementState& state) {
    return Read(state, 26u);
}
float ObjectMovementScalars::Set26(ObjectMovementState& state, float value) {
    return Write(state, 26u, value);
}
float ObjectMovementScalars::Get27(const ObjectMovementState& state) {
    return Read(state, 27u);
}
float ObjectMovementScalars::Set27(ObjectMovementState& state, float value) {
    return Write(state, 27u, value);
}

}  // namespace pal4
