#pragma once

#include "pal4/config_value_lookup.h"
#include "pal4/script_attribute_tree_lookup.h"
#include "pal4/string_buffer.h"

#include <cstdint>

namespace pal4 {

class IXmlNodeAttributeReaderRuntime {
public:
    virtual ~IXmlNodeAttributeReaderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CreateQuery(
        std::uint32_t key_address, std::uint8_t incoming_byte) = 0;
    virtual void DestroyQuery(std::uint32_t query_address) = 0;
    [[nodiscard]] virtual IScriptAttributeTreeRuntime& TreeRuntime() = 0;
    [[nodiscard]] virtual IStringAssignRuntime& Strings() = 0;
    virtual void AttributeToString(std::uint32_t field_address,
                                   ConfigStringState& output,
                                   std::uint32_t fallback_address) = 0;
    [[nodiscard]] virtual std::uint32_t StringLength(
        std::uint32_t address) = 0;
};

class XmlNodeAttributeReader final {
public:
    // IDA 0x432960. The caller owns the returned string buffer.
    [[nodiscard]] static ConfigStringState& ReadString(
        std::uint32_t document_address, ConfigStringState& output,
        std::uint32_t key_address, std::uint32_t fallback_address,
        IXmlNodeAttributeReaderRuntime& runtime);
};

}  // namespace pal4
