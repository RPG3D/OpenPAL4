#pragma once

#include <cstdint>

namespace pal4 {

class MemoryCopy final {
public:
    static void Copy(void* destination, const void* source,
                     std::uint32_t bytes);
    // IDA 0x4d4fb0: generic copy that returns the destination pointer.
    [[nodiscard]] static void* CopyAndReturn(
        void* destination, const void* source, std::uint32_t bytes);
    // IDA 0x4edf00: copy one DWORD only when the destination is nonnull.
    [[nodiscard]] static std::uint32_t* CopyWordIfDestinationNonNull(
        std::uint32_t* destination, const std::uint32_t* source);
    // IDA 0x4ee720: copy one byte and return its source pointer.
    [[nodiscard]] static const std::uint8_t* CopyByteAndReturnSource(
        std::uint8_t* destination, const std::uint8_t* source);
};

}  // namespace pal4
