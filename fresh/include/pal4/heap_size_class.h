#pragma once

#include <cstdint>

namespace pal4 {

class HeapSizeClass final {
public:
    // IDA 0x402830: maps a block size to the allocator's large-bin index.
    [[nodiscard]] static std::uint32_t Index(std::uint32_t block_size);
};

}  // namespace pal4
