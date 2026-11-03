#include "pal4/strip_frame.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::array<float, 3>, 3> kAxes{{
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}}};

std::array<float, 3> Vector(const StripObjectState& state,
                            const std::uint32_t offset) {
    return {std::bit_cast<float>(state.words[offset / 4u]),
            std::bit_cast<float>(state.words[offset / 4u + 1u]),
            std::bit_cast<float>(state.words[offset / 4u + 2u])};
}

}  // namespace

std::uint8_t StripFrame::Rebuild(
    const StripObjectState& state, IStripFrameRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto frame = state.words[33u];
    const auto scale = Vector(state, 64u);
    const auto rotation = Vector(state, 76u);
    const auto position = Vector(state, 88u);
    const auto rotate_first = (state.words[15u] & 0xffu) != 0u;
    runtime.SetIdentity(frame);
    if (!rotate_first)
        runtime.Scale(frame, scale, 0u);
    for (std::uint32_t i = 0u; i < 3u; ++i)
        runtime.Rotate(frame, kAxes[i], rotation[i],
                       rotate_first && i == 0u ? 0u : 2u);
    if (rotate_first)
        runtime.Scale(frame, scale, 2u);
    runtime.Translate(frame, position, 2u);
    return runtime.UpdateObjects(frame);
}

}  // namespace pal4
