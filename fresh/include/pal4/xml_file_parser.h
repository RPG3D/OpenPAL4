#pragma once

#include "pal4/xml_parser_document.h"

#include <cstdint>

namespace pal4 {

class XmlFileParser final {
public:
    static constexpr std::uint32_t kEmptyErrorText = 0x8dc780u;

    // IDA 0x5f0e50..0x5f0f90.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t parser, IXmlDocumentRuntime& runtime);
    static void Cleanup(std::uint32_t parser,
                        IXmlDocumentRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneInto(
        std::uint32_t source, std::uint32_t destination,
        IXmlDocumentRuntime& runtime);
    [[nodiscard]] static std::uint32_t CloneNew(
        std::uint32_t source, IXmlDocumentRuntime& runtime);
};

}  // namespace pal4
