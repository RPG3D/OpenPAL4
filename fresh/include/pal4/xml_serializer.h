#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace pal4 {

class IXmlSerializationRuntime {
public:
    virtual ~IXmlSerializationRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::string ReadText(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t StoreTemporary(
        std::string_view text) = 0;
    [[nodiscard]] virtual std::uint32_t FreeTemporary(
        std::uint32_t block) = 0;
    [[nodiscard]] virtual int WriteVirtual(std::uint32_t node,
                                           std::FILE* stream, int depth,
                                           std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t NextSibling(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual bool IsInline(std::uint32_t node,
                                        std::uint32_t vtable_offset) = 0;
};

class XmlSerializationRuntimeStub final : public IXmlSerializationRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    std::string ReadText(std::uint32_t address) override;
    std::uint32_t StoreTemporary(std::string_view text) override;
    std::uint32_t FreeTemporary(std::uint32_t block) override;
    int WriteVirtual(std::uint32_t node, std::FILE* stream,
                     int depth, std::uint32_t vtable_offset) override;
    std::uint32_t NextSibling(std::uint32_t node) override;
    bool IsInline(std::uint32_t node,
                  std::uint32_t vtable_offset) override;
};

class XmlSerializer final {
public:
    // IDA 0x5f0b40 / 0x5f0bc0.
    [[nodiscard]] static std::uint32_t WriteAttribute(
        std::uint32_t attribute, std::FILE* stream, int depth,
        IXmlSerializationRuntime& runtime);
    [[nodiscard]] static int WriteNode(
        std::uint32_t node, std::FILE* stream, int depth,
        IXmlSerializationRuntime& runtime);
    // IDA 0x5f1010.
    [[nodiscard]] static int WriteChildren(
        std::uint32_t node, std::FILE* stream, int depth,
        IXmlSerializationRuntime& runtime);
    // IDA 0x5f10b0.
    [[nodiscard]] static std::uint32_t WriteAttributeRecord(
        std::uint32_t attribute, std::FILE* stream, int depth,
        std::string* appended, IXmlSerializationRuntime& runtime);
    // IDA 0x5f1370.
    [[nodiscard]] static int WriteCData(
        std::uint32_t node, std::FILE* stream, int depth,
        IXmlSerializationRuntime& runtime);
    // IDA 0x5f1860.
    [[nodiscard]] static int WriteOpenTag(
        std::uint32_t node, std::FILE* stream, int depth,
        IXmlSerializationRuntime& runtime);
};

}  // namespace pal4
