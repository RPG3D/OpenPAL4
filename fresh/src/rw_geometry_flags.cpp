#include "pal4/rw_geometry_flags.h"

namespace pal4 {

std::uint32_t RwGeometryFlags::SetAtomicFlags45(
    const std::uint32_t atomic, const std::uint32_t,
    IRwGeometryFlagsRuntime& runtime) {
    const auto geometry = runtime.ReadGeometryWord(atomic + 24);
    if (geometry != 0) {
        const auto address = geometry + 8;
        runtime.WriteGeometryWord(
            address, runtime.ReadGeometryWord(address) | 0x45u);
    }
    return atomic;
}

std::uint32_t RwGeometryFlags::SetClumpFlags45(
    const std::uint32_t clump, IRwGeometryFlagsRuntime& runtime) {
    return runtime.ForAllGeometryFlagAtomics(
        clump, &SetAtomicFlags45, 0);
}

}  // namespace pal4
