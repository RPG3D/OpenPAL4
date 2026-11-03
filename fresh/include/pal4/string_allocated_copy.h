#pragma once

#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

class IStringAllocatedCopyRuntime {
public:
    virtual ~IStringAllocatedCopyRuntime() = default;
    virtual void CopyCString(std::uint32_t destination,
                             std::uint32_t source) = 0;
};

class StringAllocatedCopy final {
public:
    StringAllocatedCopy(IStringBufferRuntime& strings,
                        IStringAllocatedCopyRuntime& copy)
        : strings_(strings), copy_(copy) {}

    // 0x5eaf50: duplicate a C string through a temporary StringBuffer.
    [[nodiscard]] std::uint32_t CopyFromCString(
        std::uint32_t& destination, std::uint32_t source);

private:
    IStringBufferRuntime& strings_;
    IStringAllocatedCopyRuntime& copy_;
};

}  // namespace pal4
