#include "pal4/effect_system_object_base.h"

namespace pal4 {

EffectSystemObjectBaseState& EffectSystemObjectBase::Construct(
    EffectSystemObjectBaseState& state) {
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    state.words[4u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

void EffectSystemObjectBase::RestoreVtable(
    EffectSystemObjectBaseState& state) {
    state.words[0u] = kVtable;
}

}  // namespace pal4
