#include "pal4/memory_region_chain.h"

namespace pal4 {

std::uint32_t MemoryRegionChain::Push(
    const std::uint32_t base, const std::uint32_t reserve_size,
    IMemoryRegionChainRuntime& runtime) {
    const std::uint32_t node = runtime.HeapAllocate(
        runtime.GetProcessHeap(), 0u, 20u);
    if (node == 0u) {
        return 0u;
    }

    runtime.Write(node, base);
    runtime.Write(node + 4u, base);
    runtime.Write(node + 12u, reserve_size);
    runtime.Write(node + 8u, base + reserve_size);
    runtime.Write(node + 16u, head_);
    head_ = node;
    return 1u;
}

std::uint32_t MemoryRegionChain::Pop(
    IMemoryRegionChainRuntime& runtime) {
    const std::uint32_t node = head_;
    const std::uint32_t previous = runtime.Read(node + 16u);
    const std::uint32_t result = runtime.HeapFree(
        runtime.GetProcessHeap(), 0u, node);
    if (result == 0u) {
        return 0u;
    }
    head_ = previous;
    return 1u;
}

}  // namespace pal4
