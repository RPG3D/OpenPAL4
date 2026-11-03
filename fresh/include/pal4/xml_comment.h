#pragma once

#include "pal4/xml_leaf_runtime.h"

#include <cstdint>
#include <cstdio>

namespace pal4 {

class IXmlParserNodeRuntime;
class IXmlSerializationRuntime;

class XmlComment final {
public:
    static constexpr std::uint32_t kVtable = 0x849a58u;

    // IDA 0x5f12d0 / 0x5f12e0 / 0x5f1320 / 0x5f1330.
    static void Cleanup(std::uint32_t comment,
                        IXmlParserNodeRuntime& parser);
    [[nodiscard]] static int WriteToFile(
        std::uint32_t comment, std::FILE* stream, int depth,
        IXmlSerializationRuntime& serialization);
    [[nodiscard]] static std::uint32_t IsComment(
        std::uint32_t comment, std::uint32_t visitor,
        IXmlLeafRuntime& runtime);
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t source, IXmlParserNodeRuntime& parser,
        IXmlLeafRuntime& runtime);
};

}  // namespace pal4
