#include "pal4/memory_page_size_cache.h"

namespace pal4 {

std::uint32_t MemoryPageSizeCache::Get(ISystemPageInfo& platform) {
    if (page_size_ == 0u) {
        page_size_ = platform.GetSystemPageSize();
    }
    return page_size_;
}

}  // namespace pal4
