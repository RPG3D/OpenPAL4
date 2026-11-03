#include "pal4/effect_facet_base.h"

#include <cstddef>

namespace pal4 {

void EffectFacetBase::Initialize(const std::uint32_t address,
                                 const EffectFacetBaseKind kind,
                                 IEffectFacetBaseRuntime& runtime) {
    runtime.WriteWord(address, kVtables[static_cast<std::size_t>(kind)]);
}

std::uint32_t EffectFacetBase::DeletingDestroy(
    const std::uint32_t address, const EffectFacetBaseKind kind,
    const std::uint32_t flags, IEffectFacetBaseRuntime& runtime) {
    Initialize(address, kind, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(address);
    return address;
}

}  // namespace pal4
