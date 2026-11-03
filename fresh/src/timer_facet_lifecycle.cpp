#include "pal4/timer_facet_lifecycle.h"

namespace pal4 {

std::uint32_t TimerFacetLifecycle::DestroyWithFlag(
    std::uint32_t facet, std::uint8_t flags, ITimerFacetRuntime& runtime) {
    runtime.WriteDword(facet, 0x843b40u);
    if ((flags & 1u) != 0u) {
        runtime.Free(facet);
    }
    return facet;
}

}  // namespace pal4
