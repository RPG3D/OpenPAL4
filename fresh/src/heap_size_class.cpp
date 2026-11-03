#include "pal4/heap_size_class.h"

namespace pal4 {

std::uint32_t HeapSizeClass::Index(const std::uint32_t block_size) {
    std::uint32_t scaled = block_size >> 8;
    if (scaled >= 0x10000u) {
        return 0x5fu;
    }

    std::uint32_t shift = ((scaled - 0x100u) >> 16) & 8u;
    scaled <<= shift;
    std::uint32_t next_shift = ((scaled - 0x1000u) >> 16) & 4u;
    scaled <<= next_shift;
    shift += next_shift;
    next_shift = ((scaled - 0x4000u) >> 16) & 2u;
    scaled <<= next_shift;
    scaled >>= 14;

    // IDA initially treated 0x40287c as data; its raw bytes decode to
    // `shr edx, 1`.
    std::uint32_t group = (~(scaled >> 1) & scaled) - next_shift - shift + 13u;
    const std::uint32_t lane = (block_size >> ((group + 6u) & 31u)) & 3u;
    return lane + group * 4u + 0x20u;
}

}  // namespace pal4
