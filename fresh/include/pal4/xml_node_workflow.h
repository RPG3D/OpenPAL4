#pragma once

#include "pal4/xml_parser_document.h"

#include <cstdint>

namespace pal4 {

class XmlNodeWorkflow final {
public:
    // IDA 0x5f0d70 / 0x5f0dc0.
    [[nodiscard]] static std::uint32_t AcceptVisitor(
        std::uint32_t node, std::uint32_t visitor,
        IXmlDocumentRuntime& runtime);
    [[nodiscard]] static std::uint32_t ParseAndClone(
        std::uint32_t node, IXmlDocumentRuntime& runtime);
    // IDA 0x5f1050.
    [[nodiscard]] static std::uint32_t AcceptVisitor2(
        std::uint32_t node, std::uint32_t visitor,
        IXmlDocumentRuntime& runtime);
};

}  // namespace pal4
