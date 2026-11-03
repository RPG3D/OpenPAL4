#pragma once

#include "pal4/xml_leaf_runtime.h"

#include <cstdint>

namespace pal4 {

class IXmlDocumentRuntime;
class IXmlParserNodeRuntime;

class XmlTextNode final {
public:
    static constexpr std::uint32_t kVtable = 0x849aa0u;
    static constexpr std::uint32_t kInitialText = 0x8dc780u;

    // IDA 0x5f1470 / 0x5f1480 / 0x5f1550.
    [[nodiscard]] static std::uint32_t AcceptVisitor(
        std::uint32_t node, std::uint32_t visitor,
        IXmlLeafRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IXmlDocumentRuntime& parser);
    static void Cleanup(std::uint32_t node,
                        IXmlParserNodeRuntime& parser);
    // IDA 0x5f37a0.
    [[nodiscard]] static bool IsWhitespaceOnly(
        std::uint32_t node, IXmlParserNodeRuntime& parser);
};

}  // namespace pal4
