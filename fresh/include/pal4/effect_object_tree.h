#pragma once

#include "pal4/monster_manager_tree_insert.h"

#include <cstdint>

namespace pal4 {

class IArenaSequenceRuntime;
class IUiTextBufferRecordRuntime;

class EffectObjectRecord final {
public:
    // IDA 0x62ed80: copy a nullable 20-byte effect text/value record.
    static void CopyTextAndValue(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
};

class IEffectObjectTreeRuntime : public IMonsterManagerTreeInsertRuntime {
public:
    ~IEffectObjectTreeRuntime() override = default;
    [[nodiscard]] std::uint32_t NodeSize() const final { return 36u; }
    [[nodiscard]] std::uint32_t ColorOffset() const final { return 32u; }
};

class EffectObjectTree final {
public:
    // IDA 0x639d60: advance and write the original iterator to result.
    [[nodiscard]] static std::uint32_t AdvanceAndCapture(
        std::uint32_t& iterator, std::uint32_t result,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x62d730: find or insert a 36-byte effect record.
    [[nodiscard]] static MonsterManagerTreeInsertResult FindOrCreate(
        std::uint32_t tree, std::uint32_t record,
        IEffectObjectTreeRuntime& runtime);
    // IDA 0x62d990: unlink and destroy one effect-object tree node.
    static void RemoveNode(std::uint32_t tree, std::uint32_t result,
                           std::uint32_t node, std::uint32_t nil,
                           IArenaSequenceRuntime& runtime);
    // IDA 0x62dda0: return the matching effect node or the tree header.
    [[nodiscard]] static std::uint32_t FindExact(
        std::uint32_t tree, std::uint32_t key,
        IEffectObjectTreeRuntime& runtime);
    // IDA 0x62de50: advance an object-tree iterator in order.
    [[nodiscard]] static std::uint32_t Advance(
        std::uint32_t& iterator, IEffectObjectTreeRuntime& runtime);
    // IDA 0x62ebb0: move an object-tree iterator to its predecessor.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t& iterator, IEffectObjectTreeRuntime& runtime);
    // IDA 0x62e100: release an object-tree branch right-first.
    [[nodiscard]] static std::uint32_t DeleteSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62e150: insert a record at a chosen parent/child link.
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectObjectTreeRuntime& runtime);
    // IDA 0x62e3c0: promote the right child.
    [[nodiscard]] static std::uint32_t PromoteRightChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectObjectTreeRuntime& runtime);
    // IDA 0x62e420: promote the left child.
    [[nodiscard]] static std::uint32_t PromoteLeftChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectObjectTreeRuntime& runtime);
};

}  // namespace pal4
