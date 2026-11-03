#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IXmlParserNodeRuntime;

class XmlNameParser final {
public:
    // IDA 0x5f1ec0. Returns the first byte after the name, or null on failure.
    [[nodiscard]] static const std::uint8_t* Read(
        const std::uint8_t* text, std::string& name, int encoding_mode);
    [[nodiscard]] static std::uint32_t ReadRuntime(
        std::uint32_t text, std::uint32_t destination,
        int encoding_mode, IXmlParserNodeRuntime& runtime);
};

}  // namespace pal4
