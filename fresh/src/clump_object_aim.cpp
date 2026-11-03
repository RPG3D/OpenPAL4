#include "pal4/clump_object_aim.h"

#include <bit>

namespace pal4 {
namespace {

bool Active(const ClumpObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

}  // namespace

std::uint8_t ClumpObjectAimFacet::SetPosition(
    const ClumpObjectState& state, const std::array<float, 3>& position,
    IClumpObjectTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    return static_cast<std::uint8_t>(
        runtime.Translate(state.words[18u], position, 0u));
}

std::uint8_t ClumpObjectAimFacet::GetPosition(
    const ClumpObjectState& state, std::array<std::uint32_t, 3>& output,
    IClumpObjectTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto manager = state.words[18u];
    for (std::uint32_t i = 0u; i < 3u; ++i)
        output[i] = runtime.ReadWord(manager + 172u + 4u * i);
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint8_t ClumpObjectAimFacet::SetTarget(
    ClumpObjectState& state, const std::array<std::uint32_t, 3>& target) {
    if (!Active(state))
        return 0u;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[20u + i] = target[i];
    return static_cast<std::uint8_t>(target[1u]);
}

std::uint8_t ClumpObjectAimFacet::SetRotationOffset(
    ClumpObjectState& state, const std::array<std::uint32_t, 3>& offset) {
    if (!Active(state))
        return 0u;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[23u + i] = offset[i];
    return static_cast<std::uint8_t>(offset[2u]);
}

std::uint8_t ClumpObjectAimFacet::UpdateFrame(
    ClumpObjectState& state, const std::uint8_t enabled,
    IClumpObjectAimRuntime& runtime) {
    if (!Active(state))
        return 0u;
    state.words[26u] = (state.words[26u] & 0xffffff00u) | enabled;
    if (enabled == 0u)
        return 0u;

    const auto manager = state.words[18u];
    std::array<std::uint32_t, 3> position{};
    std::array<std::uint32_t, 3> scale{};
    std::array<float, 3> direction{};
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        position[i] = runtime.ReadWord(manager + 172u + 4u * i);
        direction[i] = std::bit_cast<float>(state.words[20u + i]) -
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
    ClumpAimMatrix matrix{};
    matrix.right = right;
    matrix.up = up;
    matrix.at = direction;
    runtime.UpdateMatrix(matrix);

    const auto frame = runtime.GetFrame(state);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        scale[i] = runtime.ReadWord(manager + 184u + 4u * i);
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, scale, 0u);
    runtime.ApplyMatrix(frame, matrix, 2u);
    runtime.TranslateFrame(frame, position, 2u);
    std::array<float, 3> angles{};
    const std::array<std::uint32_t, 3> offset{
        state.words[23u], state.words[24u], state.words[25u]};
    runtime.UpdateFrameRotation(frame, offset, angles);
    return runtime.SetRotation(state, angles, 0u);
}

std::uint8_t ClumpObjectAimFacet::SetPositionWithMode(
    const ClumpObjectState& state, const std::array<float, 3>& position,
    const std::uint8_t mode, IClumpObjectTransformRuntime& runtime) {
    if (!Active(state))
        return 0u;
    return static_cast<std::uint8_t>(
        runtime.Translate(state.words[18u], position, mode));
}

}  // namespace pal4
