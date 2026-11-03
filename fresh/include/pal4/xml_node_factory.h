#pragma once

#include "pal4/xml_parser_node.h"

#include <cstdint>

namespace pal4 {

class IXmlNodeFactoryRuntime : public IXmlParserNodeRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t ConstructElement(
        std::uint32_t address) = 0;
};

class XmlNodeFactory final {
public:
    // IDA 0x5f26b0. The element constructor remains an injected later entry.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t owner, std::uint32_t source, int mode,
        IXmlNodeFactoryRuntime& runtime);
};

}  // namespace pal4
