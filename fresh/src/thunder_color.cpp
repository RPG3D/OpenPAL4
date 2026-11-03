#include "pal4/thunder_color.h"

namespace pal4 {

std::uint8_t ThunderColor::Get(
    const ThunderObjectState& state,
    std::array<std::uint8_t, 4u>& output) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto packed = state.words[27u];
    for (std::uint32_t i = 0u; i < output.size(); ++i)
        output[i] = static_cast<std::uint8_t>(packed >> (8u * i));
    return output[3u];
}

std::uint8_t ThunderColor::Set(
    ThunderObjectState& state,
    const std::array<std::uint8_t, 4u>& channels) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    std::uint32_t packed = 0u;
    for (std::uint32_t i = 0u; i < channels.size(); ++i)
        packed |= static_cast<std::uint32_t>(channels[i]) << (8u * i);
    state.words[27u] = packed;
    return channels[0u];
}

}  // namespace pal4
