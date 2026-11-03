#include "pal4/sound3d_properties.h"

#include <bit>

namespace pal4 {

std::uint8_t Sound3DProperties::GetPosition(
    const Sound3DObjectState& state, std::uint32_t& x,
    std::uint32_t& y, std::uint32_t& z) {
    const auto active = static_cast<std::uint8_t>(state.words[1u] & 0xffu);
    if (active == 0u)
        return active;
    x = state.words[10u];
    y = state.words[11u];
    z = state.words[12u];
    return static_cast<std::uint8_t>(z & 0xffu);
}

std::uint32_t Sound3DProperties::SetPosition(
    Sound3DObjectState& state, const float x, const float y, const float z,
    const std::uint8_t relative, ISound3DPropertyRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u] & 0xffu);
    if (active == 0u)
        return active;
    std::array<float, 3> position{x, y, z};
    if (relative != 0u) {
        for (std::uint32_t i = 0u; i < position.size(); ++i)
            position[i] = std::bit_cast<float>(state.words[10u + i]) + position[i];
    }
    for (std::uint32_t i = 0u; i < position.size(); ++i)
        state.words[10u + i] = std::bit_cast<std::uint32_t>(position[i]);

    const auto frame = runtime.GetFrame(state);
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, {1.0f, 1.0f, 1.0f}, 0u);
    runtime.RotateFrame(frame, {1.0f, 0.0f, 0.0f}, 0.0f, 2u);
    runtime.RotateFrame(frame, {0.0f, 1.0f, 0.0f}, 0.0f, 2u);
    runtime.RotateFrame(frame, {0.0f, 0.0f, 1.0f}, 0.0f, 2u);
    return runtime.TranslateFrame(frame, position, 2u);
}

}  // namespace pal4
