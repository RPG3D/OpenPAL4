#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "pal4/xml_node_data_reader.h"

namespace pal4 {

class IXmlDocumentDataSource {
public:
    virtual ~IXmlDocumentDataSource() = default;
    [[nodiscard]] virtual std::string GetAttribute(
        std::uint32_t source_node, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t FirstChild(
        std::uint32_t source_node, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t NextChild(
        std::uint32_t source_node, std::string_view name) = 0;
    // The source bridges XMLAttribute_GetType; unknown names retain its
    // pointer-valued result rather than becoming a valid type ordinal.
    [[nodiscard]] virtual std::uint32_t GetType(
        std::string_view type_name) = 0;
};

class XmlDocumentDataReader final {
public:
    // IDA 0x5f7aa0. The source is the parsed Xnd_Node XML hierarchy.
    [[nodiscard]] static std::uint8_t Read(
        std::uint32_t parent, std::uint32_t node,
        std::uint32_t source_node, IXmlDocumentDataSource& source,
        IXmlNodeDataRuntime& output);
};

}  // namespace pal4
