#pragma once

#include <cstdint>

namespace pal4 {

class IHeapZeroRuntime {
public:
    virtual ~IHeapZeroRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Zero(std::uint32_t address, std::uint32_t size) = 0;
};

class HeapZeroAllocator final {
public:
    [[nodiscard]] static std::uint32_t AllocateAndZero(
        std::uint32_t count, std::uint32_t element_size,
        IHeapZeroRuntime& runtime);
    [[nodiscard]] static std::uint32_t AllocateAndZeroWrapper(
        std::uint32_t count, std::uint32_t element_size, std::uint32_t hint,
        IHeapZeroRuntime& runtime);
};

}  // namespace pal4
