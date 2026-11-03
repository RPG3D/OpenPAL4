#pragma once

#include "pal4/xml_parser_node.h"

#include <cstdint>

namespace pal4 {

class IXmlDocumentRuntime : public IXmlParserNodeRuntime {
public:
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void DestroyAttribute(std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t NextSibling(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t CloneChild(
        std::uint32_t node, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual bool VisitorEnter(
        std::uint32_t visitor, std::uint32_t node,
        std::uint32_t attributes, std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual bool VisitChild(
        std::uint32_t child, std::uint32_t visitor,
        std::uint32_t vtable_offset) = 0;
    [[nodiscard]] virtual std::uint32_t VisitorLeave(
        std::uint32_t visitor, std::uint32_t node,
        std::uint32_t vtable_offset) = 0;
};

class XmlDocumentRuntimeStub : public IXmlDocumentRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint32_t Free(std::uint32_t address) override;
    std::uint32_t DeleteNode(std::uint32_t node, std::uint32_t flag,
                             std::uint32_t vtable_offset) override;
    std::uint32_t StringLength(std::uint32_t text) override;
    std::uint32_t AssignString(std::uint32_t destination,
                               std::uint32_t source,
                               std::uint32_t length) override;
    std::int32_t CompareText(std::uint32_t left,
                             std::uint32_t right) override;
    std::uint32_t CallVirtual(std::uint32_t node,
                              std::uint32_t vtable_offset) override;
    std::uint32_t AttributeSetup(std::uint32_t node,
                                 std::uint32_t kind,
                                 std::uint32_t first,
                                 std::uint32_t second,
                                 std::uint32_t third) override;
    std::uint32_t Allocate(std::uint32_t bytes) override;
    void DestroyAttribute(std::uint32_t root) override;
    std::uint32_t NextSibling(std::uint32_t node) override;
    std::uint32_t CloneChild(std::uint32_t node,
                             std::uint32_t vtable_offset) override;
    bool VisitorEnter(std::uint32_t visitor, std::uint32_t node,
                      std::uint32_t attributes,
                      std::uint32_t vtable_offset) override;
    bool VisitChild(std::uint32_t child, std::uint32_t visitor,
                    std::uint32_t vtable_offset) override;
    std::uint32_t VisitorLeave(std::uint32_t visitor,
                               std::uint32_t node,
                               std::uint32_t vtable_offset) override;
};

class XmlParserDocument final {
public:
    // IDA 0x5f08b0..0x5f0a10.
    [[nodiscard]] static std::uint32_t Parse(
        std::uint32_t parser, std::uint32_t source,
        IXmlDocumentRuntime& runtime);
    static void Cleanup(std::uint32_t parser,
                        IXmlDocumentRuntime& runtime);
    [[nodiscard]] static std::uint32_t CleanupNodes(
        std::uint32_t parser, IXmlDocumentRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetAttribute(
        std::uint32_t parser, std::uint32_t name,
        IXmlDocumentRuntime& runtime);
    [[nodiscard]] static std::uint32_t SetAttribute(
        std::uint32_t parser, std::uint32_t name,
        std::uint32_t value, IXmlDocumentRuntime& runtime);
    // IDA 0x5f0d00.
    [[nodiscard]] static std::uint32_t CloneInto(
        std::uint32_t source, std::uint32_t destination,
        IXmlDocumentRuntime& runtime);
};

class XmlAttributeLifecycle final {
public:
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t attribute, IXmlParserNodeRuntime& runtime);
    // IDA 0x5f0b80.
    [[nodiscard]] static std::uint32_t Cleanup(
        std::uint32_t attribute, IXmlParserNodeRuntime& runtime);
    // IDA 0x5f1960 has the same attribute-block cleanup sequence.
    [[nodiscard]] static std::uint32_t CleanupAlternate(
        std::uint32_t attribute, IXmlParserNodeRuntime& runtime);
};

class XmlAttributeRoot final {
public:
    // IDA 0x5f1930.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t root, IXmlParserNodeRuntime& runtime);
    // IDA 0x5f19a0 / 0x5f19c0 / 0x5f1a00.
    [[nodiscard]] static std::uint32_t AddChild(
        std::uint32_t root, std::uint32_t child,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RemoveChild(
        std::uint32_t root, std::uint32_t child,
        IXmlParserNodeRuntime& runtime);
    [[nodiscard]] static std::uint32_t FindByName(
        std::uint32_t root, std::uint32_t name,
        IXmlParserNodeRuntime& runtime);
};

}  // namespace pal4
