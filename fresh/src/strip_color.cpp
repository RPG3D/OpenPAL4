#include "pal4/strip_color.h"

namespace pal4 {

std::uint8_t StripColor::Get(
    const StripObjectState& state, std::array<std::uint8_t, 4>& output) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto rgba = state.words[25u];
    for (std::uint32_t i = 0u; i < 4u; ++i)
        output[i] = static_cast<std::uint8_t>(rgba >> (8u * i));
    return output[3u];
}

std::uint8_t StripColor::Set(
    StripObjectState& state, const std::array<std::uint8_t, 4>& rgba,
    IStripColorRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    std::uint32_t packed = 0u;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        packed |= static_cast<std::uint32_t>(rgba[i]) << (8u * i);
    state.words[25u] = packed;
    if (rgba[3u] == 0xffu)
        state.words[25u] = (packed & 0x00ffffffu) | 0xfe000000u;
    return runtime.SetClumpMaterialColor(state.words[34u], state.words[25u]);
}

}  // namespace pal4
