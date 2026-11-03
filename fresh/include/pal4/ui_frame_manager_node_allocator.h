#pragma once

#include <cstdint>

namespace pal4 {

class IUiFrameManagerNodeAllocatorRuntime {
public:
    virtual ~IUiFrameManagerNodeAllocatorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Allocate(
        std::uint32_t size) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
};

class UiFrameManagerNodeAllocator final {
public:
    // IDA 0x4c07c0; allocation failure has no observed guard.
    [[nodiscard]] static std::uint32_t AllocateAndInitialize(
        std::uint32_t first, std::uint32_t second,
        IUiFrameManagerNodeAllocatorRuntime& runtime);
};

}  // namespace pal4
