#pragma once

#include <cstdint>

#include "pal4/monster_manager_tree_insert.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {

class IUiTextBufferRecordRuntime;

class IEffectSystemTaskFactoryTreeRuntime
    : public IMonsterManagerTreeInsertRuntime {
public:
    ~IEffectSystemTaskFactoryTreeRuntime() override = default;
    [[nodiscard]] std::uint32_t SharedNil() const final { return 0x93cdf4u; }
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

class IEffectSystemTaskFactoryTreeLifecycleRuntime {
public:
    virtual ~IEffectSystemTaskFactoryTreeLifecycleRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address, std::uint32_t value) = 0;
    virtual void DeleteSubtree(std::uint32_t root) = 0;
    virtual void ReleaseText(std::uint32_t record) = 0;
    virtual void FreeNode(std::uint32_t node) = 0;
    [[nodiscard]] virtual std::uint32_t Advance(std::uint32_t node) = 0;
    virtual void EraseNode(std::uint32_t tree, std::uint32_t node) = 0;
};

class EffectSystemTaskFactoryTree final {
public:
    static constexpr std::uint32_t kNil = 0x93cdf4u;

    // IDA 0x641560: skip an existing name; otherwise copy and insert it.
    [[nodiscard]] static bool ProcessEvent(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x6416a0: unique-key insertion with a node/inserted result.
    [[nodiscard]] static MonsterManagerTreeInsertResult SearchAndInsert(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x6417a0: return exact matching node or the tree head.
    [[nodiscard]] static std::uint32_t FindExact(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x641850: full 36-byte red-black node insertion.
    [[nodiscard]] static MonsterManagerTreeInsertResult InsertTextNode(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x641ab0: clear entries while retaining the header and shared nil.
    [[nodiscard]] static std::uint32_t Clear(
        std::uint32_t tree,
        IEffectSystemTaskFactoryTreeLifecycleRuntime& runtime);
    // IDA 0x641b50: first node whose name is not less than the query.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t record,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x641bb0: allocate at a selected leaf and repair tree colors.
    [[nodiscard]] static std::uint32_t InsertAt(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x641e20/0x641e80: promote right/left child respectively.
    [[nodiscard]] static std::uint32_t RotateLeft(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    [[nodiscard]] static std::uint32_t RotateRight(
        std::uint32_t tree, std::uint32_t node,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x641ee0: predecessor cursor with distinct native EAX result.
    [[nodiscard]] static MonsterManagerTreeStep Previous(
        std::uint32_t node,
        IEffectSystemTaskFactoryTreeRuntime& runtime);
    // IDA 0x641f40/0x642080: copy the 20-byte name/creator record.
    [[nodiscard]] static std::uint32_t CopyRecord(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
    static void CopyRecordIfPresent(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
};

}  // namespace pal4
