#pragma once

#include <cstdint>

namespace pal4 {

class IHeapTrimRuntime {
public:
    virtual ~IHeapTrimRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ExtendProgramBreak(
        std::uint32_t amount) = 0;
};

class HeapTrim final {
public:
    [[nodiscard]] static bool Adjust(std::uint32_t padding,
                                     std::uint32_t heap_address,
                                     IHeapTrimRuntime& runtime);
};

}  // namespace pal4
