#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "pal4/xml_attribute_record.h"
#include "pal4/xml_document_record.h"

namespace pal4 {

class IXmlBinaryNodeWriteRuntime {
public:
    virtual ~IXmlBinaryNodeWriteRuntime() = default;
    virtual void Write(std::uint32_t stream, const void* data,
                       std::uint32_t size) = 0;
    [[nodiscard]] virtual std::string NodeName(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::int32_t AttributeCount(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t AttributeAt(
        std::uint32_t node, std::int32_t index) = 0;
    [[nodiscard]] virtual std::uint32_t AttributeType(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::string AttributeName(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::uint8_t AttributeChar(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::int32_t AttributeInt(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual double AttributeFloat(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::string AttributeString(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::vector<std::uint8_t> AttributeRaw(
        std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::int32_t ChildCount(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t ChildAt(
        std::uint32_t node, std::int32_t index) = 0;
};

class XmlBinaryNodeWriter final {
public:
    // IDA 0x5f8360. Writes the 1.1 header before the root node.
    [[nodiscard]] static std::uint8_t WriteConfig(
        std::uint32_t root, std::uint32_t stream,
        IXmlBinaryNodeWriteRuntime& runtime);
    // IDA 0x5f83a0. Writes one node and its descendants.
    [[nodiscard]] static std::uint8_t WriteNode(
        std::uint32_t node, std::uint32_t stream,
        IXmlBinaryNodeWriteRuntime& runtime);
};

class IXmlBinaryOutput {
public:
    virtual ~IXmlBinaryOutput() = default;
    virtual void Write(std::uint32_t stream, const void* data,
                       std::uint32_t size) = 0;
};

// Both record runtimes must address the same original 32-bit memory space.
class XmlBinaryNodeRecordAdapter final : public IXmlBinaryNodeWriteRuntime {
public:
    XmlBinaryNodeRecordAdapter(IXmlBinaryOutput& output,
                               IXmlDocumentRecordRuntime& documents,
                               IXmlAttributeRecordRuntime& attributes);
    void Write(std::uint32_t stream, const void* data,
               std::uint32_t size) override;
    [[nodiscard]] std::string NodeName(std::uint32_t node) override;
    [[nodiscard]] std::int32_t AttributeCount(std::uint32_t node) override;
    [[nodiscard]] std::uint32_t AttributeAt(
        std::uint32_t node, std::int32_t index) override;
    [[nodiscard]] std::uint32_t AttributeType(
        std::uint32_t attribute) override;
    [[nodiscard]] std::string AttributeName(
        std::uint32_t attribute) override;
    [[nodiscard]] std::uint8_t AttributeChar(
        std::uint32_t attribute) override;
    [[nodiscard]] std::int32_t AttributeInt(
        std::uint32_t attribute) override;
    [[nodiscard]] double AttributeFloat(
        std::uint32_t attribute) override;
    [[nodiscard]] std::string AttributeString(
        std::uint32_t attribute) override;
    [[nodiscard]] std::vector<std::uint8_t> AttributeRaw(
        std::uint32_t attribute) override;
    [[nodiscard]] std::int32_t ChildCount(std::uint32_t node) override;
    [[nodiscard]] std::uint32_t ChildAt(
        std::uint32_t node, std::int32_t index) override;

private:
    IXmlBinaryOutput& output_;
    IXmlDocumentRecordRuntime& documents_;
    IXmlAttributeRecordRuntime& attributes_;
};

}  // namespace pal4
