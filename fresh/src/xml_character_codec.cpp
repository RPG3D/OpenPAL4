#include "pal4/xml_character_codec.h"

#include <array>
#include <cctype>

namespace pal4 {

std::uint8_t* XmlCharacterCodec::EncodeUtf8(
    const std::uint32_t codepoint, std::uint8_t* output,
    std::uint32_t& byte_count) {
    if (codepoint >= 0x200000u) {
        byte_count = 0u;
        return nullptr;
    }
    byte_count = codepoint < 0x80u ? 1u :
                 codepoint < 0x800u ? 2u :
                 codepoint < 0x10000u ? 3u : 4u;
    constexpr std::array<std::uint8_t, 5> prefix{
        0u, 0u, 0xc0u, 0xe0u, 0xf0u};
    auto remaining = codepoint;
    for (auto index = byte_count; index > 1u; --index) {
        output[index - 1u] = static_cast<std::uint8_t>(
            (remaining & 0x3fu) | 0x80u);
        remaining >>= 6u;
    }
    output[0] = static_cast<std::uint8_t>(remaining | prefix[byte_count]);
    return output + 1u;
}

int XmlCharacterCodec::IsAlphaOrExtended(const std::uint8_t value) {
    return value >= 0x7fu ? 1 : std::isalpha(value);
}

int XmlCharacterCodec::IsAlnumOrExtended(const std::uint8_t value) {
    return value >= 0x7fu ? 1 : std::isalnum(value);
}

}  // namespace pal4
