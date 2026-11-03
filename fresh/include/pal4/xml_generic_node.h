#pragma once

#include <cstdint>

namespace pal4 {

class IXmlLeafRuntime;
class IXmlParserNodeRuntime;

class XmlGenericNode final {
public:
    static constexpr std::uint32_t kVtable = 0x849b30u;

    // IDA 0x5f18b0 / 0x5f18c0 / 0x5f1920.
    [[nodiscard]] static std::uint32_t AcceptVisitor(
        std::uint32_t node, std::uint32_t visitor,
        IXmlLeafRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IXmlParserNodeRuntime& parser,
        IXmlLeafRuntime& runtime);
    static void Cleanup(std::uint32_t node,
                        IXmlParserNodeRuntime& parser);
};

}  // namespace pal4
