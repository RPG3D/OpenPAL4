#pragma once

#include <cstdint>

namespace pal4 {

class UiTreePairData final {
public:
    // IDA 0x4c2220. A null destination leaves source unread.
    template <typename WordMemory>
    [[nodiscard]] static std::uint32_t Copy(
        std::uint32_t destination, std::uint32_t source,
        WordMemory& memory) {
        if (destination != 0u) {
            memory.WriteWord(destination, memory.ReadWord(source));
            memory.WriteWord(destination + 4u,
                             memory.ReadWord(source + 4u));
        }
        return destination;
    }
};

}  // namespace pal4
