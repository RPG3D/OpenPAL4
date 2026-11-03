#pragma once

#include <cstdint>

namespace pal4 {

class IMemoryRegionChainRuntime {
public:
    virtual ~IMemoryRegionChainRuntime() = default;
    [[nodiscard]] virtual std::uint32_t GetProcessHeap() = 0;
    [[nodiscard]] virtual std::uint32_t HeapAllocate(
        std::uint32_t heap, std::uint32_t flags, std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t HeapFree(
        std::uint32_t heap, std::uint32_t flags,
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
};

class MemoryRegionChain final {
public:
    explicit MemoryRegionChain(std::uint32_t& head) : head_(head) {}

    [[nodiscard]] std::uint32_t Push(std::uint32_t base,
                                     std::uint32_t reserve_size,
                                     IMemoryRegionChainRuntime& runtime);
    [[nodiscard]] std::uint32_t Pop(IMemoryRegionChainRuntime& runtime);

private:
    std::uint32_t& head_;
};

}  // namespace pal4
