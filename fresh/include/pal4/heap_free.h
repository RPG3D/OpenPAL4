#pragma once

#include "pal4/heap_allocator.h"

#include <cstdint>

namespace pal4 {

class IHeapFreeRuntime : public IHeapRuntime {
public:
    [[nodiscard]] virtual std::uint32_t AdjustHeapSize(
        std::uint32_t amount, std::uint32_t heap_address) = 0;
    [[nodiscard]] virtual std::uint32_t ReleaseVirtualMemory(
        std::uint32_t base, std::uint32_t size) = 0;
};

class HeapFree final {
public:
    [[nodiscard]] static std::uint32_t Free(std::uint32_t payload,
                                            IHeapFreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t MemFree(std::uint32_t payload,
                                               IHeapFreeRuntime& runtime);
};

}  // namespace pal4
