#pragma once

#include <cstdint>

#include "pal4/xml_attribute_record.h"
#include "pal4/xml_document_record.h"

namespace pal4 {

class IXmlNodeDataRuntime {
public:
    virtual ~IXmlNodeDataRuntime() = default;
    [[nodiscard]] virtual std::int32_t ReadInt32() = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte() = 0;
    virtual void ReadBytes(std::uint32_t destination, std::uint32_t size) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateTemporary(std::uint32_t size) = 0;
    virtual void FreeTemporary(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void SetName(std::uint32_t node, std::uint32_t name,
                         std::uint32_t parent) = 0;
    [[nodiscard]] virtual std::uint32_t InitializeAttribute(
        std::uint32_t type, std::uint32_t name,
        std::span<std::uint32_t, 7> storage) = 0;
    virtual void SetChar(std::uint32_t attribute, std::uint8_t value) = 0;
    virtual void SetInt(std::uint32_t attribute, std::int32_t value) = 0;
    virtual void SetFloatBits(std::uint32_t attribute,
                              std::uint32_t bits) = 0;
    virtual void SetString(std::uint32_t attribute,
                           std::uint32_t text) = 0;
    virtual void SetRaw(std::uint32_t attribute, std::uint32_t data,
                        std::uint32_t size) = 0;
    virtual void AddAttribute(std::uint32_t node,
                              std::uint32_t attribute) = 0;
    virtual void DestroyAttribute(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint32_t CreateNode() = 0;
    // A failed recursive read still owns this child, even before SetName has
    // attached it. Destruction also detaches an already attached child.
    virtual void DestroyNode(std::uint32_t node) = 0;
};

class XmlNodeDataReader final {
public:
    // IDA 0x5f77c0. SetName attaches each newly constructed child to parent.
    [[nodiscard]] static std::uint8_t Read(std::uint32_t parent,
                                            std::uint32_t node,
                                            IXmlNodeDataRuntime& runtime);
    // IDA 0x5f7e30 uses virtual slot +16 for the same byte format.
    [[nodiscard]] static std::uint8_t ReadVirtualStream(
        std::uint32_t parent, std::uint32_t node,
        IXmlNodeDataRuntime& runtime);
};

class IXmlNodeDataInput {
public:
    virtual ~IXmlNodeDataInput() = default;
    virtual void Read(void* destination, std::uint32_t size) = 0;
};

// Bridges the reader's stream and temporary values to the reconstructed
// 28-byte attribute and 60-byte document records. Both record runtimes must
// address the same local 32-bit memory space, including borrowed stack records.
// Other memory representations implement IXmlNodeDataRuntime directly.
class XmlNodeDataRecordAdapter final : public IXmlNodeDataRuntime {
public:
    XmlNodeDataRecordAdapter(IXmlNodeDataInput& input,
                             XmlDocumentTreeGlobals& globals,
                             IXmlDocumentRecordRuntime& documents,
                             IXmlAttributeRecordRuntime& attributes);

    [[nodiscard]] std::int32_t ReadInt32() override;
    [[nodiscard]] std::uint8_t ReadByte() override;
    void ReadBytes(std::uint32_t destination, std::uint32_t size) override;
    [[nodiscard]] std::uint32_t AllocateTemporary(std::uint32_t size) override;
    void FreeTemporary(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    void SetName(std::uint32_t node, std::uint32_t name,
                 std::uint32_t parent) override;
    [[nodiscard]] std::uint32_t InitializeAttribute(
        std::uint32_t type, std::uint32_t name,
        std::span<std::uint32_t, 7> storage) override;
    void SetChar(std::uint32_t attribute, std::uint8_t value) override;
    void SetInt(std::uint32_t attribute, std::int32_t value) override;
    void SetFloatBits(std::uint32_t attribute, std::uint32_t bits) override;
    void SetString(std::uint32_t attribute, std::uint32_t text) override;
    void SetRaw(std::uint32_t attribute, std::uint32_t data,
                std::uint32_t size) override;
    void AddAttribute(std::uint32_t node, std::uint32_t attribute) override;
    void DestroyAttribute(std::uint32_t attribute) override;
    [[nodiscard]] std::uint32_t CreateNode() override;
    void DestroyNode(std::uint32_t node) override;

private:
    IXmlNodeDataInput& input_;
    XmlDocumentTreeGlobals& globals_;
    IXmlDocumentRecordRuntime& documents_;
    IXmlAttributeRecordRuntime& attributes_;
};

}  // namespace pal4
