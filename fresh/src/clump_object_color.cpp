#include "pal4/clump_object_color.h"

namespace pal4 {

std::uint8_t ClumpObject::GetColor(
    const ClumpObjectState& state,
    std::array<std::uint8_t, 4>& output) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto packed = state.words[17u];
    for (std::uint32_t i = 0u; i < output.size(); ++i)
        output[i] = static_cast<std::uint8_t>(packed >> (8u * i));
    return output[3u];
}

std::uintptr_t ClumpObject::SetColor(
    ClumpObjectState& state,
    const std::array<std::uint8_t, 4>& color,
    IClumpObjectColorRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return reinterpret_cast<std::uintptr_t>(&state) + 36u;

    const auto alpha = color[3u] >= 0xfeu ? 0xfeu : color[3u];
    state.words[17u] = static_cast<std::uint32_t>(color[0u]) |
                       (static_cast<std::uint32_t>(color[1u]) << 8u) |
                       (static_cast<std::uint32_t>(color[2u]) << 16u) |
                       (static_cast<std::uint32_t>(alpha) << 24u);
    return runtime.SetColor(state.words[18u], state.words[17u]);
}

}  // namespace pal4
