#include "pal4/cylinder_object_aim.h"

#include <bit>

namespace pal4 {

std::uint8_t CylinderObjectAim::GetPosition(
    const CylinderObjectState& state,
    std::array<std::uint32_t, 3>& output,
    ICylinderObjectAimRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i < output.size(); ++i)
        output[i] = runtime.ReadWord(state.words[22u] + 36u + 4u * i);
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint8_t CylinderObjectAim::SetRotationOffset(
    CylinderObjectState& state,
    const std::array<std::uint32_t, 3>& offset) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i < offset.size(); ++i)
        state.words[18u + i] = offset[i];
    return static_cast<std::uint8_t>(offset[2u]);
}

std::uint8_t CylinderObjectAim::UpdateFrame(
    CylinderObjectState& state, const std::uint8_t enabled,
    ICylinderObjectAimFrameRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    state.words[21u] = (state.words[21u] & 0xffffff00u) | enabled;
    if (enabled == 0u)
        return 0u;

    const auto model = state.words[22u];
    std::array<std::uint32_t, 3> position{};
    std::array<std::uint32_t, 3> scale{};
    std::array<float, 3> direction{};
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        position[i] = runtime.ReadWord(model + 36u + 4u * i);
        direction[i] = std::bit_cast<float>(state.words[15u + i]) -
                       std::bit_cast<float>(position[i]);
    }
    runtime.Normalize(direction);
    std::array<float, 3> right{direction[2u], 0.0f, -direction[0u]};
    runtime.Normalize(right);
    std::array<float, 3> up{
        direction[1u] * right[2u] - direction[2u] * right[1u],
        direction[2u] * right[0u] - direction[0u] * right[2u],
        direction[0u] * right[1u] - direction[1u] * right[0u]};
    runtime.Normalize(up);
    CylinderAimMatrix matrix{};
    matrix.right = right;
    matrix.up = up;
    matrix.at = direction;
    runtime.UpdateMatrix(matrix);

    const auto frame = runtime.GetFrame(state);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        scale[i] = runtime.ReadWord(model + 24u + 4u * i);
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, scale, 0u);
    runtime.ApplyMatrix(frame, matrix, 2u);
    runtime.TranslateFrame(frame, position, 2u);
    std::array<float, 3> angles{};
    const std::array<std::uint32_t, 3> offset{
        state.words[18u], state.words[19u], state.words[20u]};
    runtime.UpdateFrameRotation(frame, offset, angles);
    return runtime.SetRotation(state, angles, 0u);
}

std::uint8_t CylinderObjectAim::SetPositionWithMode(
    const CylinderObjectState& state,
    const std::array<float, 3>& position,
    const std::uint8_t mode,
    ICylinderObjectTransformRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.SetPosition(state.words[22u], position, mode);
}

}  // namespace pal4
