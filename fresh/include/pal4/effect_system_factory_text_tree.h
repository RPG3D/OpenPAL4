#pragma once

#include <cstdint>
#include <string_view>

#include "pal4/monster_manager_tree_insert.h"

namespace pal4 {

class IUiTextBufferRecordRuntime;

struct EffectFactoryTreeEntry {
    std::string_view name;
    std::uint32_t creator;
};

struct EffectFactoryTreeInsertResult {
    std::uint32_t node;
    bool inserted;
};

struct EffectFactoryTreeStep {
    std::uint32_t next;
    std::uint32_t result;
};

class IEffectSystemFactoryTreeRuntime {
public:
    virtual ~IEffectSystemFactoryTreeRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint8_t ReadByte(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual int Compare(
        std::string_view input, std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Previous(
        std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent,
        EffectFactoryTreeEntry entry) = 0;
};

class IEffectSystemFactoryTextInsertRuntime
    : public IMonsterManagerTreeInsertRuntime {
public:
    ~IEffectSystemFactoryTextInsertRuntime() override = default;
    [[nodiscard]] std::uint32_t SharedNil() const final {
        return 0x93cdf0u;
    }
    [[nodiscard]] std::uint32_t NodeSize() const final { return 36u; }
    [[nodiscard]] std::uint32_t ColorOffset() const final { return 32u; }
    [[nodiscard]] std::int32_t CompareRange(
        std::uint32_t input, std::uint32_t node_text) final;
    [[nodiscard]] std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) final;
    void AssignText(std::uint32_t destination,
                    std::uint32_t source) final;
    void SetNodeData(MonsterManagerTreeInsertResult& result,
                     std::uint32_t candidate) final;
    [[nodiscard]] virtual std::int32_t CompareNames(
        std::uint32_t first, std::uint32_t second) = 0;
    virtual void CopyFactoryRecord(std::uint32_t destination,
                                   std::uint32_t source) = 0;
};

class IEffectSystemFactoryTextLifecycleRuntime {
public:
    virtual ~IEffectSystemFactoryTextLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DeleteSubtree(std::uint32_t root) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Next(std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t tree, std::uint32_t node) = 0;
};

class IEffectSystemFactoryTextLookupRuntime {
public:
    virtual ~IEffectSystemFactoryTextLookupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::int32_t CompareText(
        std::uint32_t first, std::uint32_t second) = 0;
};

class IEffectSystemFactoryTextCursorRuntime {
public:
    virtual ~IEffectSystemFactoryTextCursorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class EffectSystemFactoryTextTree final {
public:
    static constexpr std::uint32_t kNil = 0x93cdf0u;

    // IDA 0x63e910: unique insertion through the later node-insert helper.
    [[nodiscard]] static EffectFactoryTreeInsertResult SearchAndInsert(
        std::uint32_t tree, EffectFactoryTreeEntry entry,
        IEffectSystemFactoryTreeRuntime& runtime);
    // IDA 0x63ea10: direct text-tree insertion with red-black repair.
    [[nodiscard]] static EffectFactoryTreeInsertResult InsertTextNode(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemFactoryTextInsertRuntime& runtime);
    // IDA 0x63ee20: positioned 36-byte node insertion and red-black repair.
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectSystemFactoryTextInsertRuntime& runtime);
    // IDA 0x63f090/0x63f0f0: promote the right/left child.
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemFactoryTextInsertRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemFactoryTextInsertRuntime& runtime);
    // IDA 0x63ec70: clear nodes but retain the tree head and shared nil.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t tree,
        IEffectSystemFactoryTextLifecycleRuntime& runtime);
    // IDA 0x63ed10: return the matching node or the tree head.
    [[nodiscard]] static std::uint32_t FindExact(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemFactoryTextLookupRuntime& runtime);
    // IDA 0x63edc0: first name not less than the requested name.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemFactoryTextLookupRuntime& runtime);
    // IDA 0x63f150: in-order successor with distinct cursor/EAX results.
    [[nodiscard]] static EffectFactoryTreeStep Next(
        std::uint32_t node,
        IEffectSystemFactoryTextCursorRuntime& runtime);
    // IDA 0x63f1b0: copy the 20-byte name/creator record.
    static void CopyTextBuffer(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
};

}  // namespace pal4
