#pragma once

#include "pal4/monster_manager_tree_insert.h"

#include <cstdint>

namespace pal4 {

class IArenaSequenceRuntime;

// This tree instantiation uses dword_93CDA8 and Dialog_TextBufferCopy2.
// The runtime binds those native resources to the shared red-black algorithm.
class IDialogTextTreeRuntime : public IMonsterManagerTreeInsertRuntime {
public:
    ~IDialogTextTreeRuntime() override = default;
};

class DialogTextTree final {
public:
    // IDA 0x62ab50: insert into the ordered 36-byte-node text tree.
    [[nodiscard]] static MonsterManagerTreeInsertResult Insert(
        std::uint32_t tree, std::uint32_t input,
        IDialogTextTreeRuntime& runtime);
    // IDA 0x62adb0: erase one node and write its successor to result.
    static void EraseNode(std::uint32_t tree, std::uint32_t result,
                          std::uint32_t node, std::uint32_t nil,
                          IArenaSequenceRuntime& runtime);
    // IDA 0x62b1c0: erase an iterator range or clear the complete tree.
    static void EraseRange(std::uint32_t tree, std::uint32_t result,
                           std::uint32_t first, std::uint32_t last,
                           std::uint32_t nil,
                           IArenaSequenceRuntime& runtime);
    // IDA 0x62b630: first node whose text key is not less than input.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t input,
        IDialogTextTreeRuntime& runtime);
    // IDA 0x62b690: release a tree branch right-first along its left spine.
    [[nodiscard]] static std::uint32_t DeleteSubtree(
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62b6e0: allocate, link and rebalance a unique-key node.
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t input,
        IDialogTextTreeRuntime& runtime);
    // IDA 0x62b950: promote a node's right child.
    [[nodiscard]] static std::uint32_t PromoteRightChild(
        std::uint32_t tree, std::uint32_t node,
        IDialogTextTreeRuntime& runtime);
    // IDA 0x62b9b0: return the minimum node below a child link.
    [[nodiscard]] static std::uint32_t MinimumFromLink(
        std::uint32_t link, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62b9d0: promote a node's left child.
    [[nodiscard]] static std::uint32_t PromoteLeftChild(
        std::uint32_t tree, std::uint32_t node,
        IDialogTextTreeRuntime& runtime);
    // IDA 0x62c420: move to the previous in-order text node.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t node, IDialogTextTreeRuntime& runtime);
    // IDA 0x62c650: move to the next in-order text node.
    [[nodiscard]] static std::uint32_t Next(
        std::uint32_t node, IDialogTextTreeRuntime& runtime);
};

}  // namespace pal4
