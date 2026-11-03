#include "pal4/effect_property_state.h"

#include <bit>

namespace pal4 {

void EffectPropertyLifecycle::Clear(EffectPropertyState& state) {
    state.words[16u] = 0u;
    state.words[17u] = 0u;
    state.words[19u] = 0u;
}

EffectPropertyState& EffectPropertyLifecycle::Reset(
    EffectPropertyState& state) {
    Clear(state);
    constexpr auto ten = std::bit_cast<std::uint32_t>(10.0f);
    state.words[5u] = ten;
    state.words[4u] = ten;
    state.words[3u] = ten;
    return state;
}

}  // namespace pal4
