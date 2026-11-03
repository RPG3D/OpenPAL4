#include "pal4/effect_facet_a830.h"

namespace pal4 {

void EffectFacetA830::Initialize(EffectFacetA830Storage& facet) {
    facet.vtable = kVtable;
}

std::uintptr_t EffectFacetA830::DeletingDestroy(
    EffectFacetA830Storage& facet, const std::uint32_t flags,
    IEffectFacetA830Runtime& runtime) {
    facet.vtable = kVtable;
    if ((flags & 1u) != 0u)
        runtime.Free(facet);
    return reinterpret_cast<std::uintptr_t>(&facet);
}

}  // namespace pal4
