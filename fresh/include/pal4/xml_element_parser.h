#pragma once

#include "pal4/xml_node_factory.h"

#include <array>
#include <cstdint>

namespace pal4 {

class IXmlElementRuntime : public IXmlNodeFactoryRuntime {
public:
    [[nodiscard]] virtual std::uint32_t ParseNode(
        std::uint32_t node, std::uint32_t source,
        std::array<std::uint32_t, 4>* position, int mode) = 0;
};

class XmlElementParser final {
public:
    // IDA 0x5f2950.
    [[nodiscard]] static std::uint32_t Parse(
        std::uint32_t element, std::uint32_t source,
        std::array<std::uint32_t, 4>* position, int mode,
        IXmlElementRuntime& runtime);
};

}  // namespace pal4
