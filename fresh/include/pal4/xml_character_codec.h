#pragma once

#include <cstdint>

namespace pal4 {

class XmlCharacterCodec final {
public:
    // IDA 0x5f1ac0. Caller provides at least four writable bytes.
    [[nodiscard]] static std::uint8_t* EncodeUtf8(
        std::uint32_t codepoint, std::uint8_t* output,
        std::uint32_t& byte_count);
    // IDA 0x5f1bc0 / 0x5f1be0.
    [[nodiscard]] static int IsAlphaOrExtended(std::uint8_t value);
    [[nodiscard]] static int IsAlnumOrExtended(std::uint8_t value);
};

}  // namespace pal4
