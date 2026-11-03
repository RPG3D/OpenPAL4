#include "pal4/thunder_base_facet.h"

namespace pal4 {

void ThunderBaseFacet::InstallVtable(std::uint32_t& vtable_word) {
    vtable_word = kVtable;
}

std::uint32_t ThunderBaseFacet::DeletingDestructor(
    std::uint32_t& vtable_word, const std::uint32_t facet_address,
    const std::uint32_t flags, IThunderBaseFacetRuntime& runtime) {
    InstallVtable(vtable_word);
    if ((flags & 1u) != 0u)
        runtime.Free(facet_address);
    return facet_address;
}

}  // namespace pal4
