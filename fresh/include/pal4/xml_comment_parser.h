#pragma once

#include "pal4/xml_node_factory.h"

#include <array>
#include <cstdint>

namespace pal4 {

class XmlCommentParser final {
public:
    // IDA 0x5f2f60 (named XML_ParseCData in the database).
    [[nodiscard]] static std::uint32_t Parse(
        std::uint32_t node, std::uint32_t source,
        std::array<std::uint32_t, 4>* position, int mode,
        IXmlNodeFactoryRuntime& runtime);
};

}  // namespace pal4
