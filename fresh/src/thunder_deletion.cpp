#include "pal4/thunder_deletion.h"

namespace pal4 {

std::uint32_t ThunderDeletion::Delete(
    ThunderObjectState& state, const std::uint32_t object_address,
    const std::uint32_t flags, IThunderDeletionRuntime& runtime) {
    runtime.Destroy(state);
    if ((flags & 1u) != 0u)
        runtime.Free(object_address);
    return object_address;
}

std::uint32_t ThunderDeletion::DeleteFromFacet(
    ThunderObjectState& state, const std::uint32_t facet_address,
    const std::uint32_t facet_offset, const std::uint32_t flags,
    IThunderDeletionRuntime& runtime) {
    return Delete(state, facet_address - facet_offset, flags, runtime);
}

}  // namespace pal4
