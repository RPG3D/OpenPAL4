#pragma once

#include "pal4/script_event_processor.h"
#include "pal4/xml_document_record.h"

#include <span>
#include <memory>
#include <string>
#include <string_view>

namespace pal4 {

// Native storage for event documents and event names used by game objects.
class NativeEventDocumentRuntime final : public IScriptEventProcessorRuntime,
                                         private IXmlDocumentRecordRuntime {
public:
    explicit NativeEventDocumentRuntime(IStringBufferStorageRuntime& memory);
    NativeEventDocumentRuntime(const NativeEventDocumentRuntime&) = delete;
    NativeEventDocumentRuntime& operator=(const NativeEventDocumentRuntime&) = delete;

    std::uint32_t Allocate(std::uint32_t bytes) override;
    std::uint32_t InitializeDocument(std::uint32_t address, const char* name,
                                     std::uint32_t option) override;
    // IDA5F5030 default construction, without a named-string constructor.
    [[nodiscard]] std::uint32_t InitializeEmptyDocument(std::uint32_t address);
    void CleanupDocument(std::uint32_t address) override;
    void DestroyDocument(std::uint32_t address) override;
    std::uint32_t ReadDword(std::uint32_t address) override;
    void CleanupEventData(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t AddEvent(
        ScriptEventProcessorState& state, const char* name);
    // IDA42D230: populate a stack node, append its copy, destroy the temporary.
    // Array growth can invalidate previously returned node addresses.
    [[nodiscard]] std::uint32_t AppendBehaviorAtomic(std::uint32_t event,
        std::int32_t class_id, std::int32_t proportion);
    void SetAtomicNodeNames(std::uint32_t event,
                            std::span<const std::string_view> names);
    void AddAtomicIntAttribute(std::uint32_t event, std::uint32_t index,
                               std::string_view name, std::int32_t value);
    void AddAtomicFloatAttribute(std::uint32_t event, std::uint32_t index,
                                 std::string_view name, float value);
    std::uint32_t Free(std::uint32_t address) override;
    [[nodiscard]] std::uint32_t FindNumericAttribute(std::uint32_t node,
                                                     std::string_view name);
    [[nodiscard]] std::int32_t AttributeToInt(std::uint32_t attribute, std::int32_t fallback);
    [[nodiscard]] float AttributeToFloat(std::uint32_t attribute, float fallback);
    // Owns the returned text. Raw attributes retain an unsupported conversion
    // boundary because the original raw-to-string branch corrupts a pointer.
    [[nodiscard]] std::string AttributeToString(std::uint32_t attribute);
    // Native behavior-memory documents, including owner+0x668. Existing keys
    // use the original attribute setter; absent keys use document insertion.
    void WriteIntAttribute(std::uint32_t node, std::string_view name,
                            std::int32_t value);
    void WriteFloatAttribute(std::uint32_t node, std::string_view name,
                              float value);
    void WriteStringAttribute(std::uint32_t node, std::string_view name,
                               std::string_view value);
    void SetAttributeStringValue(std::uint32_t attribute, std::string_view value);
    void SetAttributeIntValue(std::uint32_t attribute,std::int32_t value);
    [[nodiscard]] std::string_view NodeName(std::uint32_t node);
    [[nodiscard]] std::uint32_t ChildCount(std::uint32_t node);
    [[nodiscard]] std::uint32_t ChildAt(std::uint32_t node, std::uint32_t index);
    [[nodiscard]] std::uint32_t FindChild(std::uint32_t node, std::string_view name);
    [[nodiscard]] std::uint32_t NextChild(std::uint32_t node, std::uint32_t previous);
    // Replaces an initialized document from versioned game binary XML.
    // Invalid/truncated input is rejected before modifying the destination.
    [[nodiscard]] bool LoadBinaryDocument(std::uint32_t document,
        std::span<const std::uint8_t> bytes);
    // Independent native tree. Caller must DestroyDocument, then Free the root.
    [[nodiscard]] std::uint32_t CloneDocument(std::uint32_t source);
    void AssignDocument(std::uint32_t destination,std::uint32_t source);
    // The parent owns the new child and destroys it with its subtree.
    [[nodiscard]] std::uint32_t CreateChild(std::uint32_t parent, const char* name);

    [[nodiscard]] std::uint32_t SharedNil() const noexcept {
        return globals_.nil_node;
    }
    // References owned by this facade, rather than the shared allocation domain.
    [[nodiscard]] std::uint32_t DocumentCount() const noexcept {
        return owned_document_count_;
    }
    // Original XML construction count across the allocation domain.
    [[nodiscard]] std::uint32_t SharedDocumentCount() const noexcept {
        return globals_.construction_count;
    }

private:
    class AttributeWriterRuntime;
    class BehaviorAtomicRuntime;
    std::uint8_t InsertNativeAttribute(std::uint32_t node,
                                        std::uint32_t attribute);
    std::uint32_t AllocateMemory(std::uint32_t bytes) override;
    std::uint32_t FreeMemory(std::uint32_t address) override;
    std::uint32_t AllocateSharedNil() override;
    std::uint32_t FreeSharedNil(std::uint32_t address) override;
    void DocumentReferenceAdded() noexcept override { ++owned_document_count_; }
    void DocumentReferenceRemoved() noexcept override { --owned_document_count_; }
    void WriteWord(std::uint32_t address, std::uint32_t value) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    void WriteByte(std::uint32_t address, std::uint8_t value) override;
    std::uint8_t ReadByte(std::uint32_t address) override;
    void SetErrorFlag(std::uint32_t record, std::uint8_t flag) override;
    void Reallocate(std::uint32_t record, std::uint32_t length) override;
    void AddChild(std::uint32_t parent, std::uint32_t child) override;
    void DeleteNode(std::uint32_t node) override;
    std::uint32_t GetNextNode(std::uint32_t node) override;
    void RemoveNode(std::uint32_t document, std::uint32_t node) override;
    void GetFirstElement(std::uint32_t document) override;
    std::uint32_t EmptyTextAddress() override;
    std::uint32_t CopyAttributeToTemporary(std::uint32_t source,
        std::span<std::uint32_t, 7> storage) override;
    void DestroyTemporaryAttribute(std::uint32_t temporary) override;
    void CleanupNode(std::uint32_t node) override;
    void FreeAttributeValue(std::uint32_t attribute) override;
    void ClearAttributeTree(std::uint32_t tree, std::uint32_t first,
                            std::uint32_t head) override;
    std::uint8_t AllocateTextBuffer(std::uint32_t node,
                                    std::uint32_t length) override;
    void ReallocateTextIfNeeded(std::uint32_t node) override;
    void DetachTextBuffer(std::uint32_t node) override;
    std::uint8_t SetTextSize(std::uint32_t node,
                             std::uint32_t length) override;
    std::uint32_t FindAttributeNode(std::uint32_t tree,
                                    std::string_view name) override;
    std::uint32_t FindAttributeNodeForAddition(std::uint32_t tree,
                                    std::string_view name) override;
    std::uint32_t InsertAttributeNode(std::uint32_t tree,
        std::string_view name, std::uint32_t source) override;
    void DestroyEmbeddedAttribute(std::uint32_t attribute) override;
    std::int32_t CompareAttributeRange(std::uint32_t input,
                                       std::uint32_t node_text) override;
    std::int32_t CompareAttributeText(std::uint32_t first,
                                      std::uint32_t second) override;
    void CopyEmbeddedAttribute(std::uint32_t destination,
                               std::uint32_t source) override;

    void AddAtomicNumericAttribute(std::uint32_t event, std::uint32_t index,
                                   std::string_view name, std::uint32_t type,
                                   std::uint32_t bits);

    IStringBufferStorageRuntime& memory_;
    IStringBufferStorageRuntime& shared_storage_;
    std::shared_ptr<XmlDocumentTreeGlobals> shared_globals_;
    XmlDocumentTreeGlobals& globals_;
    std::uint32_t owned_document_count_{};
};

}  // namespace pal4
