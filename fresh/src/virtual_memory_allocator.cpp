#include "pal4/virtual_memory_allocator.h"

namespace pal4 {
namespace {

void InitializeCaches(const std::uint32_t page_address,
                      const std::uint32_t granularity_address,
                      IVirtualMemoryAllocatorRuntime& runtime) {
    if (runtime.Read(page_address) == 0u) {
        runtime.Write(page_address, runtime.PageSize());
    }
    if (runtime.Read(granularity_address) == 0u) {
        runtime.Write(granularity_address, runtime.AllocationGranularity());
    }
}

}  // namespace

std::uint32_t VirtualMemoryAllocator::Allocate(
    const std::uint32_t address, const std::uint32_t size,
    [[maybe_unused]] const std::uint32_t protection,
    [[maybe_unused]] const std::uint32_t mapping_type,
    [[maybe_unused]] const std::uint32_t handle,
    [[maybe_unused]] const std::uint32_t offset,
    IVirtualMemoryAllocatorRuntime& runtime) {
    InitializeCaches(0x8dd818u, 0x8dd828u, runtime);
    const std::uint32_t result = runtime.VirtualAllocate(
        address, size, 0x103000u, 4u);
    return result == 0u ? kFailure : result;
}

std::uint32_t VirtualMemoryAllocator::Release(
    const std::uint32_t address, [[maybe_unused]] const std::uint32_t size,
    IVirtualMemoryAllocatorRuntime& runtime) {
    InitializeCaches(0x8dd830u, 0x8dd82cu, runtime);
    return runtime.VirtualRelease(address, 0u, 0x8000u) ? 0u : kFailure;
}

}  // namespace pal4
