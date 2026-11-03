#pragma once

#include <cstdint>

namespace pal4 {

struct VirtualMemoryRegion {
    std::uint32_t base = 0;
    std::uint32_t size = 0;
    std::uint32_t state = 0;
};

class IMemoryHeapExtendRuntime {
public:
    virtual ~IMemoryHeapExtendRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t PageSize() = 0;
    [[nodiscard]] virtual std::uint32_t AllocationGranularity() = 0;
    [[nodiscard]] virtual bool InitializeStructure(std::uint32_t base,
                                                   std::uint32_t reserve_size) = 0;
    [[nodiscard]] virtual bool FreeStructure() = 0;
    [[nodiscard]] virtual std::uint32_t VirtualAllocate(
        std::uint32_t address, std::uint32_t size,
        std::uint32_t allocation_type, std::uint32_t protection) = 0;
    [[nodiscard]] virtual bool VirtualRelease(std::uint32_t address,
                                              std::uint32_t size,
                                              std::uint32_t free_type) = 0;
    [[nodiscard]] virtual bool VirtualQuery(std::uint32_t address,
                                            VirtualMemoryRegion& region) = 0;
    [[nodiscard]] virtual std::uint32_t LastError() = 0;
};

class MemoryHeapExtend final {
public:
    static constexpr std::uint32_t kFailure = 0xffffffffu;
    [[nodiscard]] static std::uint32_t Change(std::uint32_t signed_size,
                                              IMemoryHeapExtendRuntime& runtime);
};

}  // namespace pal4
