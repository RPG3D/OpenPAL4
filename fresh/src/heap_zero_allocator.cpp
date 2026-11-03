#include "pal4/heap_zero_allocator.h"

namespace pal4 {

std::uint32_t HeapZeroAllocator::AllocateAndZeroWrapper(
    const std::uint32_t count, const std::uint32_t element_size,
    const std::uint32_t hint,
    IHeapZeroRuntime& runtime) {
    (void)hint;
    return AllocateAndZero(count, element_size, runtime);
}

std::uint32_t HeapZeroAllocator::AllocateAndZero(
    const std::uint32_t count, const std::uint32_t element_size,
    IHeapZeroRuntime& runtime) {
    const std::uint32_t payload = runtime.Allocate(count * element_size);
    if (payload == 0u) {
        return 0u;
    }
    const std::uint32_t header = runtime.Read(payload - 4u);
    if ((header & 2u) == 0u) {
        runtime.Zero(payload, (header & ~3u) - 4u);
    }
    return payload;
}

}  // namespace pal4
