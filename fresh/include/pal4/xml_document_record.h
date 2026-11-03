#pragma once

#include <cstdint>
#include <span>
#include <string_view>

namespace pal4 {

struct XmlDocumentTreeGlobals {
    std::uint32_t nil_node{};             // IDA 0x8e3dac.
    std::uint32_t construction_count{};   // IDA 0x90f978.
};

class IXmlDocumentRecordRuntime {
public:
    virtual ~IXmlDocumentRecordRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateMemory(std::uint32_t bytes) = 0;
    [[nodiscard]] virtual std::uint32_t FreeMemory(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t AllocateSharedNil() = 0;
    [[nodiscard]] virtual std::uint32_t FreeSharedNil(std::uint32_t address) = 0;
    virtual void DocumentReferenceAdded() noexcept = 0;
    virtual void DocumentReferenceRemoved() noexcept = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(std::uint32_t address) = 0;
    virtual void SetErrorFlag(std::uint32_t record, std::uint8_t flag) = 0;
    virtual void Reallocate(std::uint32_t record, std::uint32_t length) = 0;
    virtual void AddChild(std::uint32_t parent, std::uint32_t child) = 0;
    virtual void DeleteNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t GetNextNode(std::uint32_t node) = 0;
    virtual void RemoveNode(std::uint32_t document, std::uint32_t node) = 0;
    virtual void GetFirstElement(std::uint32_t document) = 0;
    [[nodiscard]] virtual std::uint32_t EmptyTextAddress() = 0;
    // IDA5F57E3 uses a caller-owned 28-byte stack record. Only its copied
    // value may allocate; DestroyTemporaryAttribute must not free this storage.
    [[nodiscard]] virtual std::uint32_t CopyAttributeToTemporary(
        std::uint32_t source, std::span<std::uint32_t, 7> storage) = 0;
    virtual void DestroyTemporaryAttribute(std::uint32_t temporary) = 0;
    virtual void CleanupNode(std::uint32_t node) = 0;
    virtual void FreeAttributeValue(std::uint32_t attribute) = 0;
    virtual void ClearAttributeTree(std::uint32_t tree,
                                    std::uint32_t first,
                                    std::uint32_t head) = 0;
    [[nodiscard]] virtual std::uint8_t AllocateTextBuffer(
        std::uint32_t node, std::uint32_t length) = 0;
    virtual void ReallocateTextIfNeeded(std::uint32_t node) = 0;
    virtual void DetachTextBuffer(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint8_t SetTextSize(
        std::uint32_t node, std::uint32_t length) = 0;
    [[nodiscard]] virtual std::uint32_t FindAttributeNode(
        std::uint32_t tree, std::string_view name) = 0;
    // AddAttribute owns a separate temporary lookup key (IDA5F5C60).
    [[nodiscard]] virtual std::uint32_t FindAttributeNodeForAddition(
        std::uint32_t tree, std::string_view name) = 0;
    // Complete pair/default-attribute cleanup and value assignment while the
    // insertion key remains alive; return only after releasing that key.
    [[nodiscard]] virtual std::uint32_t InsertAttributeNode(
        std::uint32_t tree, std::string_view name, std::uint32_t source) = 0;
    virtual void DestroyEmbeddedAttribute(std::uint32_t attribute) = 0;
    [[nodiscard]] virtual std::int32_t CompareAttributeRange(
        std::uint32_t input, std::uint32_t node_text) = 0;
    [[nodiscard]] virtual std::int32_t CompareAttributeText(
        std::uint32_t first, std::uint32_t second) = 0;
    virtual void CopyEmbeddedAttribute(std::uint32_t destination,
                                       std::uint32_t source) = 0;
};

class XmlDocumentRecord final {
public:
    // IDA 0x5f5030.
    [[nodiscard]] static std::uint32_t Initialize(
        std::uint32_t document, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f50f0.
    [[nodiscard]] static std::uint32_t InitializeFromString(
        std::uint32_t document, std::uint32_t source,
        std::uint32_t parent, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5280.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t document, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5340.
    [[nodiscard]] static std::uint32_t CopyConstruct(
        std::uint32_t destination, std::uint32_t source,
        XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5460.
    [[nodiscard]] static std::uint32_t CopyAssign(
        std::uint32_t destination, std::uint32_t source,
        XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5560.
    [[nodiscard]] static std::uint32_t DestroyNode(
        std::uint32_t node, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f56b0.
    [[nodiscard]] static std::uint8_t SetName(
        std::uint32_t node, std::uint32_t name, std::uint32_t parent,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5770.
    [[nodiscard]] static std::uint32_t Clone(
        std::uint32_t source, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5890.
    [[nodiscard]] static std::uint8_t AssignWithResult(
        std::uint32_t destination, std::uint32_t source,
        XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f59b0.
    [[nodiscard]] static std::uint8_t Cleanup(
        std::uint32_t node, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5ba0.
    [[nodiscard]] static std::uint32_t GetAttributeByIndex(
        std::uint32_t node, std::uint32_t index_address,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5c60.
    [[nodiscard]] static std::uint8_t AddAttribute(
        std::uint32_t node, std::uint32_t attribute,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5e70.
    [[nodiscard]] static std::uint32_t GetChildByIndex(
        std::uint32_t node, std::uint32_t index_address,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5e90.
    [[nodiscard]] static std::uint32_t FindChildByName(
        std::uint32_t node, std::uint32_t name_address,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f5f40.
    [[nodiscard]] static std::uint32_t FindNextChildByName(
        std::uint32_t node, std::uint32_t previous_child,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6000.
    [[nodiscard]] static std::uint32_t AddChild(
        std::uint32_t parent, std::uint32_t child,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f61e0.
    [[nodiscard]] static std::uint32_t CreateNode(
        std::uint32_t parent, std::uint32_t name,
        XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6240.
    [[nodiscard]] static std::uint8_t RemoveChild(
        std::uint32_t parent, std::uint32_t child,
        XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6290.
    [[nodiscard]] static std::uint32_t CleanupDocumentValue(
        std::uint32_t document, IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6300.
    [[nodiscard]] static std::uint32_t RemoveAttributeNode(
        std::uint32_t tree, std::uint32_t iterator_address,
        std::uint32_t node, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6740.
    [[nodiscard]] static std::uint32_t ClearAttributeRange(
        std::uint32_t tree, std::uint32_t result_address,
        std::uint32_t first, std::uint32_t last,
        XmlDocumentTreeGlobals& globals, IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6820.
    [[nodiscard]] static std::uint32_t AdvanceTreeIterator(
        std::uint32_t iterator_address, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6870. Result is a node word followed by an inserted byte.
    [[nodiscard]] static std::uint32_t InsertAttributeTreeNode(
        std::uint32_t tree, std::uint32_t result_address,
        std::uint32_t input, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6ad0.
    [[nodiscard]] static std::uint32_t DeleteAttributeSubtree(
        std::uint32_t node, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6b20.
    [[nodiscard]] static std::uint32_t CreateAttributeTree(
        std::uint32_t tree, XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6ba0.
    [[nodiscard]] static std::uint32_t RotateAttributeLeft(
        std::uint32_t tree, std::uint32_t node,
        const XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6c00.
    [[nodiscard]] static std::uint32_t MinimumAttributeNode(
        std::uint32_t node, const XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6c20.
    [[nodiscard]] static std::uint32_t RotateAttributeRight(
        std::uint32_t tree, std::uint32_t node,
        const XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6c80. Advances the iterator and stores its former node.
    [[nodiscard]] static std::uint32_t AdvanceAndGetPrevious(
        std::uint32_t iterator_address, std::uint32_t result_address,
        const XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6ce0.
    [[nodiscard]] static std::uint32_t InsertAttributeTreeNodeAt(
        std::uint32_t tree, std::uint32_t result_address,
        std::uint32_t child, std::uint32_t parent, std::uint32_t input,
        const XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6f50.
    [[nodiscard]] static std::uint32_t AllocateAttributeTreeNode(
        std::uint32_t parent, std::uint32_t flag,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6f70. Moves the iterator to its in-order predecessor.
    [[nodiscard]] static std::uint32_t RetreatTreeIterator(
        std::uint32_t iterator_address,
        const XmlDocumentTreeGlobals& globals,
        IXmlDocumentRecordRuntime& runtime);
    // IDA 0x5f6fd0.
    static void CopyAttributeTreePayload(
        std::uint32_t destination, std::uint32_t source,
        IXmlDocumentRecordRuntime& runtime);
};

}  // namespace pal4
