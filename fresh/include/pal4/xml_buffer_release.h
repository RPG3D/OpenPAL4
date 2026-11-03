#pragma once

#include "pal4/xml_parser_node.h"

#include <cstdint>

namespace pal4 {

class XmlBufferRelease final {
public:
    // IDA 0x5f3780.
    [[nodiscard]] static std::uint32_t Release(
        std::uint32_t object, IXmlParserNodeRuntime& runtime);
};

}  // namespace pal4
