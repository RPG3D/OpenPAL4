#include "pal4/effect_system_object_f_color.h"

namespace pal4 {
namespace {

bool Active(const EffectSystemObjectFState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

}  // namespace

std::uint8_t EffectSystemObjectFColor::Get(
    const EffectSystemObjectFState& state,
    std::array<std::uint8_t, 4>& rgba,
    IEffectSystemObjectFColorRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto color = runtime.ReadWord(state.words[29u] + 48u);
    for (std::uint32_t index = 0u; index < rgba.size(); ++index)
        rgba[index] = runtime.ReadByte(color + 28u + index);
    return rgba[3u];
}

std::uint8_t EffectSystemObjectFColor::Set(
    const EffectSystemObjectFState& state,
    const std::array<std::uint8_t, 4>& rgba,
    IEffectSystemObjectFColorRuntime& runtime) {
    if (!Active(state))
        return 0u;
    std::uint32_t packed = 0u;
    for (std::uint32_t index = 0u; index < rgba.size(); ++index)
        packed |= static_cast<std::uint32_t>(rgba[index]) << (8u * index);
    const auto color = runtime.ReadWord(state.words[29u] + 48u);
    return runtime.SetColor(color, packed);
}

}  // namespace pal4
