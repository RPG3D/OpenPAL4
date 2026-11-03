#pragma once

#include "pal4/config_value_lookup.h"
#include "pal4/string_buffer.h"

#include <array>
#include <cstdint>

namespace pal4 {

struct XmlAttributeWriterTemporary {
    std::array<std::uint32_t, 7> words{};
};
static_assert(sizeof(XmlAttributeWriterTemporary) == 0x1cu);

class IXmlNodeAttributeWriterRuntime {
public:
    virtual ~IXmlNodeAttributeWriterRuntime() = default;
    [[nodiscard]] virtual IConfigValueMemory& Memory() = 0;
    [[nodiscard]] virtual IStringAssignRuntime& Strings() = 0;
    [[nodiscard]] virtual std::uint8_t SetInt(
        std::uint32_t field_address, std::uint32_t value_address) = 0;
    [[nodiscard]] virtual std::uint8_t SetFloat(
        std::uint32_t field_address, std::uint32_t value_address) = 0;
    [[nodiscard]] virtual std::uint8_t SetString(
        std::uint32_t field_address, std::uint32_t value_address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeIntAttribute(
        XmlAttributeWriterTemporary& temporary, std::uint32_t key_address,
        std::uint32_t value_address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeFloatAttribute(
        XmlAttributeWriterTemporary& temporary, std::uint32_t key_address,
        std::uint32_t value_address) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeStringAttribute(
        XmlAttributeWriterTemporary& temporary, std::uint32_t key_address,
        std::uint32_t value_address) = 0;
    [[nodiscard]] virtual std::uint8_t AddAttribute(
        std::uint32_t document_address, std::uint32_t attribute_address) = 0;
    virtual void DestroyAttribute(XmlAttributeWriterTemporary& temporary) = 0;
};

class XmlNodeAttributeWriter final {
public:
    // IDA 0x4326e0 and 0x432820. Return the setter/inserter's low EAX byte.
    [[nodiscard]] static std::uint8_t WriteInt(
        std::uint32_t document_address, std::uint32_t key_address,
        std::uint32_t value_address, IXmlNodeAttributeWriterRuntime& runtime);
    [[nodiscard]] static std::uint8_t WriteFloat(
        std::uint32_t document_address, std::uint32_t key_address,
        std::uint32_t value_address, IXmlNodeAttributeWriterRuntime& runtime);
    // IDA 0x579950.
    [[nodiscard]] static std::uint8_t WriteString(
        std::uint32_t document_address, std::uint32_t key_address,
        std::uint32_t value_address, IXmlNodeAttributeWriterRuntime& runtime);
};

}  // namespace pal4
