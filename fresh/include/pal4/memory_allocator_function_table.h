#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

class MemoryAllocatorFunctionTable final {
public:
    static constexpr std::uint32_t kAllocAddress = 0x408140u;
    static constexpr std::uint32_t kFreeAddress = 0x5fc6c0u;
    static constexpr std::uint32_t kReallocAddress = 0x408150u;
    static constexpr std::uint32_t kZeroAllocAddress = 0x408170u;

    [[nodiscard]] MemoryAllocatorFunctionTable& Initialize();
    [[nodiscard]] const std::array<std::uint32_t, 4>& Entries() const {
        return entries_;
    }

private:
    std::array<std::uint32_t, 4> entries_{};
};

}  // namespace pal4
