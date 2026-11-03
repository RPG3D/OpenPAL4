#include "pal4/cylinder_object_transform.h"

namespace pal4 {
namespace {

bool Active(const CylinderObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

std::uint8_t ReadVector(
    const CylinderObjectState& state,
    std::array<std::uint32_t, 3>& output,
    const std::uint32_t model_offset,
    ICylinderObjectTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto address = state.words[22u] + model_offset;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        output[i] = runtime.ReadWord(address + 4u * i);
    return static_cast<std::uint8_t>(output[2u]);
}

}  // namespace

std::uint8_t CylinderObjectTransform::GetRotation(
    const CylinderObjectState& state,
    std::array<std::uint32_t, 3>& output,
    ICylinderObjectTransformRuntime& runtime) {
    return ReadVector(state, output, 12u, runtime);
}

std::uint8_t CylinderObjectTransform::GetScale(
    const CylinderObjectState& state,
    std::array<std::uint32_t, 3>& output,
    ICylinderObjectTransformRuntime& runtime) {
    return ReadVector(state, output, 24u, runtime);
}

std::uint8_t CylinderObjectTransform::GetPosition(
    const CylinderObjectState& state,
    std::array<std::uint32_t, 3>& output,
    ICylinderObjectTransformRuntime& runtime) {
    return ReadVector(state, output, 36u, runtime);
}

std::uint8_t CylinderObjectTransform::SetScale(
    const CylinderObjectState& state,
    const std::array<float, 3>& value, const std::uint8_t mode,
    ICylinderObjectTransformRuntime& runtime) {
    return Active(state) ? runtime.SetScale(state.words[22u], value, mode) : 0u;
}

std::uint8_t CylinderObjectTransform::SetRotation(
    const CylinderObjectState& state,
    const std::array<float, 3>& value, const std::uint8_t mode,
    ICylinderObjectTransformRuntime& runtime) {
    return Active(state) ? runtime.SetRotation(state.words[22u], value, mode) : 0u;
}

std::uint8_t CylinderObjectTransform::SetPosition(
    const CylinderObjectState& state,
    const std::array<float, 3>& value, const std::uint8_t mode,
    ICylinderObjectTransformRuntime& runtime) {
    return Active(state) ? runtime.SetPosition(state.words[22u], value, mode) : 0u;
}

}  // namespace pal4
