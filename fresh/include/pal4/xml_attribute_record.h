#pragma once

#include <cstdint>
#include <string>

namespace pal4 {

class IXmlAttributeRecordRuntime {
public:
    virtual ~IXmlAttributeRecordRuntime() = default;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetErrorFlag(std::uint32_t record, std::uint8_t flag) = 0;
    virtual void Reallocate(std::uint32_t record, std::uint32_t length) = 0;
    [[nodiscard]] virtual bool AllocateText(std::uint32_t record,
                                            std::uint32_t length,
                                            bool null_terminated) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(std::uint32_t bytes) = 0;
    virtual void AssignName(std::uint32_t destination,
                            std::uint32_t source,
                            std::uint32_t start,
                            std::uint32_t end) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateRaw(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FreeBlock(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t FreeRaw(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t FloatToInt(std::uint32_t bits) = 0;
    [[nodiscard]] virtual std::uint8_t ScanChar(
        std::uint32_t text, std::uint8_t fallback) = 0;
    [[nodiscard]] virtual std::int32_t ScanInt(
        std::uint32_t text, std::int32_t fallback) = 0;
    [[nodiscard]] virtual std::uint32_t ScanFloatBits(
        std::uint32_t text, std::uint32_t fallback_bits) = 0;
    [[nodiscard]] virtual std::string FormatFloat(std::uint32_t bits) = 0;
    [[nodiscard]] virtual std::int32_t ParseInt(std::uint32_t text) = 0;
    [[nodiscard]] virtual std::uint32_t ParseFloatBits(
        std::uint32_t text) = 0;
};

class XmlAttributeRecord final {
public:
    // IDA 0x5f3820.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t record, IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f3910. name is a pointer in the original 32-bit address space.
    [[nodiscard]] static std::uint32_t InitializeWithValue(
        std::uint32_t record, std::uint32_t type, std::uint32_t name,
        IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f3a10 and 0x5f3b30. Both copy a word from value_address.
    [[nodiscard]] static std::uint32_t InitializeWithPointerValue(
        std::uint32_t record, std::uint32_t name,
        std::uint32_t value_address, IXmlAttributeRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t InitializeWithPointerValue2(
        std::uint32_t record, std::uint32_t name,
        std::uint32_t value_address, IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f3c50. value may be null, denoting the shared empty string.
    [[nodiscard]] static std::uint32_t InitializeWithValueAndType(
        std::uint32_t record, std::uint32_t name, std::uint32_t value,
        IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f3dd0.
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f3f70 and 0x5f3fe0.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t record, IXmlAttributeRecordRuntime& runtime);
    [[nodiscard]] static std::uint32_t FreeValue(
        std::uint32_t record, IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f4070.
    [[nodiscard]] static std::uint32_t ValueSize(
        std::uint32_t record, IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f40b0.
    [[nodiscard]] static std::uint32_t Assign(
        std::uint32_t destination, std::uint32_t source,
        IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f42a0, 0x5f4340 and 0x5f43e0.
    [[nodiscard]] static std::uint8_t ToChar(
        std::uint32_t record, std::uint32_t fallback_address,
        IXmlAttributeRecordRuntime& runtime);
    [[nodiscard]] static std::int32_t ToInt(
        std::uint32_t record, std::uint32_t fallback_address,
        IXmlAttributeRecordRuntime& runtime);
    [[nodiscard]] static double ToFloat(
        std::uint32_t record, std::uint32_t fallback_address,
        IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f4490.
    [[nodiscard]] static std::uint32_t ToString(
        std::uint32_t record, std::uint32_t output,
        std::uint32_t fallback_address, IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f4730.
    [[nodiscard]] static std::uint32_t GetValue(
        std::uint32_t record, std::uint32_t output,
        std::uint32_t capacity_address, IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f4810, 0x5f49b0 and 0x5f4b50.
    [[nodiscard]] static std::uint8_t SetChar(
        std::uint32_t record, std::uint32_t source,
        IXmlAttributeRecordRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetInt(
        std::uint32_t record, std::uint32_t source,
        IXmlAttributeRecordRuntime& runtime);
    [[nodiscard]] static std::uint8_t SetFloat(
        std::uint32_t record, std::uint32_t source,
        IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f4d00.
    [[nodiscard]] static std::uint8_t SetString(
        std::uint32_t record, std::uint32_t source,
        IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f4e70.
    [[nodiscard]] static std::uint8_t SetValue(
        std::uint32_t record, std::uint32_t source,
        std::uint32_t size_address, IXmlAttributeRecordRuntime& runtime);
    // IDA 0x5f72b0. Unknown names return the source text address.
    [[nodiscard]] static std::uint32_t GetTypeName(
        std::uint32_t record, IXmlAttributeRecordRuntime& runtime);
};

}  // namespace pal4
