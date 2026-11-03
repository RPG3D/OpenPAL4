#pragma once

#include "pal4/monster_manager_tree_insert.h"

#include <cstdint>

namespace pal4 {

class IEffectObjectPoolNameCountRuntime;
class IArenaSequenceRuntime;
class IUiTextBufferRecordRuntime;

class EffectNameCountRecord final {
public:
    // IDA 0x62eee0: copy a nullable 20-byte name/count record.
    static void CopyTextAndCount(
        std::uint32_t destination, std::uint32_t source,
        IUiTextBufferRecordRuntime& runtime);
};

class EffectNameCountTree final {
public:
    // IDA 0x639c80: erase a half-open range of name/count nodes.
    [[nodiscard]] static std::uint32_t EraseRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x639dc0: descend to the leftmost node from a link.
    [[nodiscard]] static std::uint32_t MinimumFromLink(
        std::uint32_t link, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62dea0: find or insert a 36-byte name/count record.
    [[nodiscard]] static MonsterManagerTreeInsertResult FindOrCreate(
        std::uint32_t tree, std::uint32_t record,
        IEffectObjectPoolNameCountRuntime& runtime);
    // IDA 0x62e480: unlink and destroy one name/count node.
    static void RemoveNode(std::uint32_t tree, std::uint32_t result,
                           std::uint32_t node, std::uint32_t nil,
                           IArenaSequenceRuntime& runtime);
    // IDA 0x62e8f0: release a name/count branch right-first.
    [[nodiscard]] static std::uint32_t DeleteSubtree(
        std::uint32_t root, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62e940: allocate and link a name/count node at a supplied parent.
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t record,
        IEffectObjectPoolNameCountRuntime& runtime);
    // IDA 0x62ec10: promote the right child of a name/count node.
    [[nodiscard]] static std::uint32_t PromoteRightChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectObjectPoolNameCountRuntime& runtime);
    // IDA 0x62ec70: promote the left child of a name/count node.
    [[nodiscard]] static std::uint32_t PromoteLeftChild(
        std::uint32_t tree, std::uint32_t node,
        IEffectObjectPoolNameCountRuntime& runtime);
    // IDA 0x62ecd0: move a name/count iterator to its predecessor.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t& iterator,
        IEffectObjectPoolNameCountRuntime& runtime);
    // IDA 0x62ed30: move a name/count iterator to its successor.
    [[nodiscard]] static std::uint32_t Advance(
        std::uint32_t& iterator,
        IEffectObjectPoolNameCountRuntime& runtime);
    // IDA 0x62e890: find the first name/count node not less than a key.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t key,
        IEffectObjectPoolNameCountRuntime& runtime);
};

}  // namespace pal4
