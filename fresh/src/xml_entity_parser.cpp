#include "pal4/xml_entity_parser.h"

#include "pal4/xml_character_codec.h"

#include <array>
#include <cstring>

namespace pal4 {

namespace {

struct NamedEntity {
    const char* spelling;
    std::size_t length;
    std::uint8_t value;
};

constexpr std::array<NamedEntity, 4> kNamedEntities{{
    {"&amp;", 5u, '&'}, {"&lt;", 4u, '<'},
    {"&gt;", 4u, '>'}, {"&quot;", 6u, '"'},
}};

int Digit(const std::uint8_t character, const bool hex) {
    if (character >= '0' && character <= '9') return character - '0';
    if (hex && character >= 'a' && character <= 'f')
        return character - 'a' + 10;
    if (hex && character >= 'A' && character <= 'F')
        return character - 'A' + 10;
    return -1;
}

}  // namespace

const std::uint8_t* XmlEntityParser::Parse(
    const std::uint8_t* text, std::uint8_t* output,
    std::uint32_t& byte_count, const int utf8_mode) {
    byte_count = 0u;
    if (text == nullptr) return nullptr;

    if (text[1] == '#' && text[2] != 0u) {
        const bool hex = text[2] == 'x';
        const auto digits = text + (hex ? 3u : 2u);
        if (hex && *digits == 0u) return nullptr;
        const auto end = reinterpret_cast<const std::uint8_t*>(
            std::strchr(reinterpret_cast<const char*>(digits), ';'));
        if (end == nullptr) return nullptr;
        std::uint32_t value = 0u;
        for (auto current = digits; current < end; ++current) {
            const auto digit = Digit(*current, hex);
            if (digit < 0) return nullptr;
            value = value * (hex ? 16u : 10u) +
                    static_cast<std::uint32_t>(digit);
        }
        if (utf8_mode == 1) {
            (void)XmlCharacterCodec::EncodeUtf8(value, output, byte_count);
        } else {
            output[0] = static_cast<std::uint8_t>(value);
            byte_count = 1u;
        }
        return end + 1u;
    }

    for (const auto& entity : kNamedEntities) {
        if (std::strncmp(entity.spelling,
                         reinterpret_cast<const char*>(text),
                         entity.length) == 0) {
            output[0] = entity.value;
            byte_count = 1u;
            return text + entity.length;
        }
    }
    output[0] = text[0];
    return text + 1u;
}

}  // namespace pal4
