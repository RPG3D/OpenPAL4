#include "pal4/memory_allocation_granularity_cache.h"

namespace pal4 {

std::uint32_t MemoryAllocationGranularityCache::Get(
    ISystemAllocationGranularity& platform) {
    if (granularity_ == 0u) {
        granularity_ = platform.GetSystemAllocationGranularity();
    }
    return granularity_;
}

}  // namespace pal4
