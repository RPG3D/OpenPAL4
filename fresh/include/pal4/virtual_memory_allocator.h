#pragma once

#include <cstdint>

namespace pal4 {

class IVirtualMemoryAllocatorRuntime {
public:
    virtual ~IVirtualMemoryAllocatorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t PageSize() = 0;
    [[nodiscard]] virtual std::uint32_t AllocationGranularity() = 0;
    [[nodiscard]] virtual std::uint32_t VirtualAllocate(
        std::uint32_t address, std::uint32_t size,
        std::uint32_t allocation_type, std::uint32_t protection) = 0;
    [[nodiscard]] virtual bool VirtualRelease(
        std::uint32_t address, std::uint32_t size,
        std::uint32_t free_type) = 0;
};

class VirtualMemoryAllocator final {
public:
    static constexpr std::uint32_t kFailure = 0xffffffffu;

    [[nodiscard]] static std::uint32_t Allocate(
        std::uint32_t address, std::uint32_t size, std::uint32_t protection,
        std::uint32_t mapping_type, std::uint32_t handle,
        std::uint32_t offset,
        IVirtualMemoryAllocatorRuntime& runtime);
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t address, std::uint32_t size,
        IVirtualMemoryAllocatorRuntime& runtime);
};

}  // namespace pal4
