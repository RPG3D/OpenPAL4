#pragma once

#include "pal4/xml_node_factory.h"

#include <array>
#include <cstdint>

namespace pal4 {

class XmlAttributeParser final {
public:
    // IDA 0x5f3030.
    [[nodiscard]] static std::uint32_t Parse(
        std::uint32_t attribute, std::uint32_t source,
        std::array<std::uint32_t, 4>* position, int mode,
        IXmlNodeFactoryRuntime& runtime);
};

}  // namespace pal4
