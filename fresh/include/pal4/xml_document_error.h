#pragma once

#include "pal4/xml_parser_node.h"

#include <array>
#include <cstdint>

namespace pal4 {

class XmlDocumentError final {
public:
    // IDA 0x5f2640. Error messages are read from the original pointer table.
    [[nodiscard]] static std::uint8_t Set(
        std::uint32_t document, std::uint32_t code,
        std::uint32_t end, std::uint32_t cursor,
        int utf8_mode, IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetWithPosition(
        std::uint32_t document, std::uint32_t code,
        std::uint32_t end, std::array<std::uint32_t, 4>* position,
        int utf8_mode, IXmlParserNodeRuntime& runtime);
};

}  // namespace pal4
