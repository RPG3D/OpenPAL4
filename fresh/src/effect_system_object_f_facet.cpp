#include "pal4/effect_system_object_f_facet.h"

namespace pal4 {

std::uint8_t EffectSystemObjectFFacet::EnableVisibility(std::uint32_t& status) {
    const auto active = static_cast<std::uint8_t>(status);
    if (active == 0u) return active;
    status = (status & 0x0000ffffu) | 0x01010000u;
    return 1u;
}

std::uint8_t EffectSystemObjectFFacet::Start(
    EffectSystemObjectFState& state,
    IEffectSystemObjectFFacetRuntime& runtime) {
    runtime.InitializeFacet(state, 20u);
    return EnableVisibility(state.words[1u]);
}

std::uint8_t EffectSystemObjectFFacet::Stop(
    EffectSystemObjectFState& state,
    IEffectSystemObjectFFacetRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;

    state.words[1u] &= 0x0000ffffu;
    const auto object = state.words[29u];
    const auto first_component = runtime.ReadWord(object + 48u);
    const auto first_clump = runtime.ReadWord(first_component + 8u);
    const auto world = runtime.FindWorld(first_clump);
    if (world == 0u)
        return 0u;
    const auto component = runtime.ReadWord(object + 48u);
    const auto clump = runtime.ReadWord(component + 8u);
    return static_cast<std::uint8_t>(runtime.RemoveClump(world, clump));
}

}  // namespace pal4
