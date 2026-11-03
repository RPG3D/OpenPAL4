#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace pal4 {

class IEffectSystemTextManagerRuntime {
public:
    virtual ~IEffectSystemTextManagerRuntime() = default;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t Allocate(std::uint32_t bytes) = 0;
    virtual void Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilReferences() = 0;
    virtual void SetSharedNilReferences(std::uint32_t count) = 0;
};

class IEffectSystemTextManagerCleanupRuntime {
public:
    virtual ~IEffectSystemTextManagerCleanupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void ClearTextElements(std::uint32_t manager) = 0;
    virtual void DeleteSubtree(std::uint32_t root) = 0;
    [[nodiscard]] virtual std::uint32_t Next(std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t manager, std::uint32_t node) = 0;
    virtual void GetFirstElement(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::uint32_t Free(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void SetSharedNil(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNilReferences() = 0;
    virtual void SetSharedNilReferences(std::uint32_t count) = 0;
};

class IEffectSystemTextManagerLookupRuntime {
public:
    virtual ~IEffectSystemTextManagerLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t LowerBound(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual int CompareName(
        std::string_view name, std::uint32_t node_record) = 0;
};

class IEffectSystemTextManagerClearRuntime {
public:
    virtual ~IEffectSystemTextManagerClearRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t SharedNil() = 0;
    virtual void CleanupProperty(std::uint32_t property) = 0;
    [[nodiscard]] virtual std::uint32_t Next(std::uint32_t node) = 0;
    virtual void DeleteSubtree(std::uint32_t root) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t manager, std::uint32_t node) = 0;
};

struct EffectSystemTextInsertResult {
    std::uint32_t node{};
    bool inserted{};
};

class IEffectSystemTextManagerEntryRuntime
    : public IEffectSystemTextManagerLookupRuntime {
public:
    ~IEffectSystemTextManagerEntryRuntime() override = default;
    [[nodiscard]] virtual std::uint32_t CloneXmlNode(
        std::uint32_t source) = 0;
    virtual void DestroyXmlNode(std::uint32_t clone) = 0;
    // A successful insertion transfers ownership of the clone to the tree.
    [[nodiscard]] virtual EffectSystemTextInsertResult Insert(
        std::uint32_t manager, std::string_view name,
        std::uint32_t clone) = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> FindAttribute(
        std::uint32_t xml_node, std::string_view attribute_name) = 0;
    virtual void AddAttribute(std::uint32_t xml_node,
                              std::string_view attribute_name,
                              std::string_view value) = 0;
    virtual void SetAttribute(std::uint32_t attribute,
                              std::string_view value) = 0;
};

class IEffectSystemTextManagerXmlRuntime {
public:
    virtual ~IEffectSystemTextManagerXmlRuntime() = default;
    virtual void ClearElements(std::uint32_t manager) = 0;
    [[nodiscard]] virtual std::string_view NodeName(
        std::uint32_t xml_node) = 0;
    [[nodiscard]] virtual std::int32_t ChildCount(
        std::uint32_t xml_node) = 0;
    [[nodiscard]] virtual std::uint32_t ChildAt(
        std::uint32_t xml_node, std::uint32_t index) = 0;
    [[nodiscard]] virtual std::string ReadAttribute(
        std::uint32_t xml_node, std::string_view name) = 0;
    [[nodiscard]] virtual bool AddEntry(
        std::uint32_t manager, std::string_view name,
        std::uint32_t xml_node) = 0;
};

class EffectSystemTextManager final {
public:
    // IDA 0x63f340. The byte originates from an uninitialized native stack slot.
    [[nodiscard]] static std::uint32_t Construct(
        std::uint32_t manager, std::uint8_t initial_byte,
        IEffectSystemTextManagerRuntime& runtime);
    // IDA 0x63f4a0: clear owned elements, head, and final shared nil.
    [[nodiscard]] static std::uint32_t Destruct(
        std::uint32_t manager,
        IEffectSystemTextManagerCleanupRuntime& runtime);
    // IDA 0x63f590/0x63f660: name lookup and node +28 property.
    [[nodiscard]] static bool Contains(
        std::uint32_t manager, std::string_view name,
        IEffectSystemTextManagerLookupRuntime& runtime);
    [[nodiscard]] static std::uint32_t GetProperty(
        std::uint32_t manager, std::string_view name,
        IEffectSystemTextManagerLookupRuntime& runtime);
    // IDA 0x63f760: release node properties, then clear the text tree.
    [[nodiscard]] static std::uint32_t ClearElements(
        std::uint32_t manager,
        IEffectSystemTextManagerClearRuntime& runtime);
    // IDA 0x63f870: insert one XML-backed entry and assign Attr_Label.
    [[nodiscard]] static bool AddEntry(
        std::uint32_t manager, std::string_view name,
        std::uint32_t source_xml,
        IEffectSystemTextManagerEntryRuntime& runtime);
    // IDA 0x63faa0: replace entries from an OxnList XML node.
    [[nodiscard]] static bool LoadFromXml(
        std::uint32_t manager, std::uint32_t xml_node,
        IEffectSystemTextManagerXmlRuntime& runtime);
};

}  // namespace pal4
