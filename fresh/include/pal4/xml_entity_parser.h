#pragma once

#include <cstdint>

namespace pal4 {

class XmlEntityParser final {
public:
    // IDA 0x5f1f60. Caller provides at least four output bytes.
    [[nodiscard]] static const std::uint8_t* Parse(
        const std::uint8_t* text, std::uint8_t* output,
        std::uint32_t& byte_count, int utf8_mode);
};

}  // namespace pal4
