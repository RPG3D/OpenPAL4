#pragma once

#include <cstdint>

namespace pal4 {

class MonsterManagerTreeNodeAllocator final {
public:
    // IDA 0x438e10 writes only parent and color after a 0x24-byte allocation.
    template <class Memory>
    [[nodiscard]] static std::uint32_t Allocate(
        std::uint32_t parent, std::uint32_t color, Memory& memory,
        std::uint32_t node_size = 0x24u,
        std::uint32_t color_offset = 0x20u) {
        const auto node = memory.AllocateMemory(node_size);
        memory.WriteDword(node + 4u, parent);
        memory.WriteDword(node + color_offset, color);
        return node;
    }
};

}  // namespace pal4
