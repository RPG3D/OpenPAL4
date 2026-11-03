#include "pal4/xml_name_parser.h"

#include "pal4/xml_character_codec.h"
#include "pal4/xml_parser_node.h"

namespace pal4 {

const std::uint8_t* XmlNameParser::Read(
    const std::uint8_t* text, std::string& name,
    [[maybe_unused]] const int encoding_mode) {
    name.clear();
    if (text == nullptr || *text == 0u ||
        (XmlCharacterCodec::IsAlphaOrExtended(*text) == 0 &&
         *text != '_'))
        return nullptr;

    auto current = text;
    while (*current != 0u &&
           (XmlCharacterCodec::IsAlnumOrExtended(*current) != 0 ||
            *current == '_' || *current == '-' || *current == '.' ||
            *current == ':'))
        ++current;
    name.assign(reinterpret_cast<const char*>(text), current - text);
    return current;
}

std::uint32_t XmlNameParser::ReadRuntime(
    const std::uint32_t text, const std::uint32_t destination,
    [[maybe_unused]] const int encoding_mode,
    IXmlParserNodeRuntime& runtime) {
    (void)runtime.AssignString(destination, 0x8dc780u, 0u);
    if (text == 0u) return 0u;
    const auto first = runtime.ReadByte(text);
    if (first == 0u ||
        (XmlCharacterCodec::IsAlphaOrExtended(first) == 0 && first != '_'))
        return 0u;
    auto current = text;
    for (;;) {
        const auto byte = runtime.ReadByte(current);
        if (byte == 0u ||
            (XmlCharacterCodec::IsAlnumOrExtended(byte) == 0 &&
             byte != '_' && byte != '-' && byte != '.' && byte != ':'))
            break;
        ++current;
    }
    (void)runtime.AssignString(destination, text, current - text);
    return current;
}

}  // namespace pal4
