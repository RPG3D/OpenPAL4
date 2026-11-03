#include "pal4/strip_aim.h"

#include "pal4/strip_properties.h"

#include <bit>

namespace pal4 {
namespace {

bool Active(const StripObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

std::array<std::uint32_t, 3> RawVector(
    const StripObjectState& state, const std::uint32_t offset) {
    return {state.words[offset / 4u], state.words[offset / 4u + 1u],
            state.words[offset / 4u + 2u]};
}

std::array<float, 3> FloatVector(
    const StripObjectState& state, const std::uint32_t offset) {
    return {std::bit_cast<float>(state.words[offset / 4u]),
            std::bit_cast<float>(state.words[offset / 4u + 1u]),
            std::bit_cast<float>(state.words[offset / 4u + 2u])};
}

}  // namespace

std::uint8_t StripAim::GetPosition(
    const StripObjectState& state,
    std::array<std::uint32_t, 3>& output) {
    if (!Active(state))
        return 0u;
    output = RawVector(state, 88u);
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint8_t StripAim::SetTarget(
    StripObjectState& state,
    const std::array<std::uint32_t, 3>& values) {
    if (!Active(state))
        return 0u;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[26u + i] = values[i];
    return static_cast<std::uint8_t>(values[1u]);
}

std::uint8_t StripAim::SetRotationOffset(
    StripObjectState& state,
    const std::array<std::uint32_t, 3>& values) {
    if (!Active(state))
        return 0u;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        state.words[29u + i] = values[i];
    return static_cast<std::uint8_t>(values[2u]);
}

std::uint32_t StripAim::SetPosition(
    StripObjectState& state, const std::array<float, 3>& values,
    const bool relative, IStripPropertyRuntime& runtime) {
    return StripProperties::SetPosition(state, values, relative, runtime);
}

std::uint32_t StripAim::UpdateFrame(
    StripObjectState& state, const std::uint8_t enabled,
    IStripAimRuntime& runtime) {
    if (!Active(state))
        return 0u;
    state.words[32u] = (state.words[32u] & 0xffffff00u) | enabled;
    if (enabled == 0u)
        return enabled;

    auto position = FloatVector(state, 88u);
    if ((state.words[35u] & 0xffu) != 0u)
        runtime.ReadStraightPosition(state.words[38u], position);
    const auto target = FloatVector(state, 104u);
    std::array<float, 3> direction{};
    for (std::uint32_t i = 0u; i < 3u; ++i)
        direction[i] = target[i] - position[i];
    runtime.Normalize(direction);

    std::array<float, 3> right{direction[2u], 0.0f, -direction[0u]};
    runtime.Normalize(right);
    std::array<float, 3> up{
        right[2u] * direction[1u] - right[1u] * direction[2u],
        right[0u] * direction[2u] - right[2u] * direction[0u],
        right[1u] * direction[0u] - right[0u] * direction[1u]};
    runtime.Normalize(up);

    StripAimMatrix matrix{};
    matrix.right = right;
    matrix.up = up;
    matrix.at = direction;
    runtime.UpdateMatrix(matrix);

    const auto frame = runtime.GetFrame(state);
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, RawVector(state, 64u), 0u);
    runtime.ApplyMatrix(frame, matrix, 2u);
    runtime.TranslateFrame(frame, RawVector(state, 88u), 2u);
    std::array<float, 3> angles{};
    runtime.UpdateFrameRotation(frame, RawVector(state, 116u), angles);
    return runtime.SetRotation(state, angles, 0u);
}

}  // namespace pal4
