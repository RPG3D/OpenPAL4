#pragma once

#include <cstdint>

namespace pal4 {

class IHeapBinStateRuntime {
public:
    virtual ~IHeapBinStateRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Read(std::uint32_t address) const = 0;
    virtual void Write(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t PageSize() = 0;
};

class HeapBinState final {
public:
    [[nodiscard]] static std::uint32_t Initialize(std::uint32_t heap_address,
                                                  IHeapBinStateRuntime& runtime);
};

}  // namespace pal4
