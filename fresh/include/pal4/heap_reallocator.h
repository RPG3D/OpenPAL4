#pragma once

#include "pal4/heap_allocator.h"

#include <cstdint>

namespace pal4 {

class IHeapReallocRuntime : public IHeapRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t size) = 0;
    virtual void Free(std::uint32_t payload) = 0;
    virtual void Copy(std::uint32_t destination, std::uint32_t source,
                      std::uint32_t size) = 0;
};

class HeapReallocator final {
public:
    [[nodiscard]] static std::uint32_t Reallocate(std::uint32_t payload,
                                                  std::uint32_t new_size,
                                                  IHeapReallocRuntime& runtime);
    [[nodiscard]] static std::uint32_t ReallocateWrapper(
        std::uint32_t payload, std::uint32_t new_size, std::uint32_t hint,
        IHeapReallocRuntime& runtime);
};

}  // namespace pal4
