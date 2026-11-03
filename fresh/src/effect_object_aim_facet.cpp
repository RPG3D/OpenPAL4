#include "pal4/effect_object_aim_facet.h"

#include <bit>

namespace pal4 {

std::uint8_t EffectObjectAimFacet::GetPosition(
    const EffectObjectState& state,
    std::array<std::uint32_t, 3u>& output,
    IEffectObjectAimFacetRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.GetPosition(state, output);
}

std::uint8_t EffectObjectAimFacet::SetPositionWithMode(
    EffectObjectState& state,
    const std::array<std::uint32_t, 3u>& values,
    const std::uint32_t mode,
    IEffectObjectAimFacetRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.SetPosition(state, values, mode);
}

std::uint8_t EffectObjectAimFacet::SetTarget(
    EffectObjectState& state,
    const std::array<std::uint32_t, 3u>& values) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i != 3u; ++i)
        state.words[26u + i] = values[i];
    return static_cast<std::uint8_t>(values[1u]);
}

std::uint8_t EffectObjectAimFacet::SetRotationOffset(
    EffectObjectState& state,
    const std::array<std::uint32_t, 3u>& values) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i != 3u; ++i)
        state.words[29u + i] = values[i];
    return static_cast<std::uint8_t>(values[2u]);
}

std::uint8_t EffectObjectAimFacet::UpdateFrame(
    EffectObjectState& state, const std::uint8_t enabled,
    IEffectObjectAimRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    state.words[32u] = (state.words[32u] & 0xffffff00u) | enabled;
    if (enabled == 0u)
        return 0u;

    std::array<std::uint32_t, 3u> position{};
    runtime.GetPosition(state, position);
    std::array<float, 3u> direction{};
    for (std::uint32_t i = 0u; i != 3u; ++i)
        direction[i] = std::bit_cast<float>(state.words[26u + i]) -
                       std::bit_cast<float>(position[i]);
    runtime.Normalize(direction);

    std::array<float, 3u> right{
        direction[2u], 0.0f, -direction[0u]};
    runtime.Normalize(right);
    std::array<float, 3u> up{
        right[2u] * direction[1u] - right[1u] * direction[2u],
        right[0u] * direction[2u] - right[2u] * direction[0u],
        right[1u] * direction[0u] - right[0u] * direction[1u]};
    runtime.Normalize(up);

    EffectObjectAimMatrix matrix{};
    matrix.right = right;
    matrix.up = up;
    matrix.at = direction;
    runtime.UpdateMatrix(matrix);

    const auto frame = runtime.GetFrame(state);
    const std::array<std::uint32_t, 3u> scale{
        state.words[13u], state.words[14u], state.words[15u]};
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, scale, 0u);
    runtime.ApplyMatrix(frame, matrix, 2u);
    runtime.TranslateFrame(frame, position, 2u);
    const std::array<std::uint32_t, 3u> offset{
        state.words[29u], state.words[30u], state.words[31u]};
    std::array<float, 3u> angles{};
    runtime.UpdateFrameRotation(frame, offset, angles);
    return runtime.SetRotation(state, angles, 0u);
}

}  // namespace pal4
