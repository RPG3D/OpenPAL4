#pragma once

#include <cstdint>

namespace pal4 {

class IHeapExtensionRuntime {
public:
    virtual ~IHeapExtensionRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ManageBlocks(std::uint32_t heap_address) = 0;
    [[nodiscard]] virtual std::uint32_t RetryAllocate(std::uint32_t request) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateVirtualMemory(
        std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t ExtendProgramBreak(
        std::uint32_t size) = 0;
    virtual void FreeBlock(std::uint32_t payload) = 0;
};

class HeapExtender final {
public:
    [[nodiscard]] static std::uint32_t Allocate(std::uint32_t block_size,
                                                std::uint32_t heap_address,
                                                IHeapExtensionRuntime& runtime);
};

}  // namespace pal4
