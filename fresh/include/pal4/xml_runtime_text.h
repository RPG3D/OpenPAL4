#pragma once

#include "pal4/xml_node_factory.h"

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class XmlRuntimeText final {
public:
    [[nodiscard]] static std::uint32_t SkipWhitespaceAndBom(
        std::uint32_t source, int mode, IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static bool HasPrefix(
        std::uint32_t source, const char* prefix,
        bool ignore_case, IXmlParserNodeRuntime& runtime);
    static void Advance(std::array<std::uint32_t, 4>& position,
                        std::uint32_t end, int mode,
                        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static bool AssignDecoded(
        std::uint32_t destination, std::string_view text,
        IXmlNodeFactoryRuntime& runtime);
    [[nodiscard]] static std::uint32_t ParseDecoded(
        std::uint32_t source, std::uint32_t destination,
        const char* delimiter, int mode, IXmlNodeFactoryRuntime& runtime,
        bool normalize = false);
};

}  // namespace pal4
