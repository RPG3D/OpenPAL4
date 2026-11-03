#include "pal4/sound3d_base_playback_facet.h"

namespace pal4 {

void Sound3DBasePlaybackFacet::Initialize(
    Sound3DBasePlaybackFacetStorage& facet) {
    facet.vtable = kVtable;
}

std::uintptr_t Sound3DBasePlaybackFacet::DeletingDestroy(
    Sound3DBasePlaybackFacetStorage& facet, const std::uint32_t flags,
    ISound3DBasePlaybackFacetRuntime& runtime) {
    facet.vtable = kVtable;
    if ((flags & 1u) != 0u)
        runtime.Free(facet);
    return reinterpret_cast<std::uintptr_t>(&facet);
}

}  // namespace pal4
