#pragma once

#include "pal4/monster_manager_tree_insert.h"

#include <cstdint>

namespace pal4 {

class IArenaSequenceRuntime;

class IDialogTargetTreeRuntime : public IMonsterManagerTreeInsertRuntime {
public:
    ~IDialogTargetTreeRuntime() override = default;
    [[nodiscard]] std::uint32_t NodeSize() const final { return 44u; }
    [[nodiscard]] std::uint32_t ColorOffset() const final { return 40u; }
};

class IDialogTargetTreeCursorRuntime {
public:
    virtual ~IDialogTargetTreeCursorRuntime() = default;
    [[nodiscard]] virtual std::uint32_t SharedNil() const = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
};

class DialogTargetTree final {
public:
    // IDA 0x62b2a0: insert a target name into the 44-byte-node tree.
    [[nodiscard]] static MonsterManagerTreeInsertResult Insert(
        std::uint32_t tree, std::uint32_t input,
        IDialogTargetTreeRuntime& runtime);
    // IDA 0x62b500: erase a half-open range or clear the entire tree.
    [[nodiscard]] static std::uint32_t EraseRange(
        std::uint32_t tree, std::uint32_t result,
        std::uint32_t first, std::uint32_t last,
        std::uint32_t nil, IArenaSequenceRuntime& runtime);
    // IDA 0x62b5e0: advance an in-order iterator to its successor.
    static void Advance(std::uint32_t& iterator,
                        IDialogTargetTreeCursorRuntime& runtime);
    // IDA 0x62bd30: unlink, rebalance and release one 44-byte target node.
    static void EraseNode(std::uint32_t tree, std::uint32_t result,
                          std::uint32_t node, std::uint32_t nil,
                          IArenaSequenceRuntime& runtime);
    // IDA 0x62c140: release a target-tree branch right-first.
    [[nodiscard]] static std::uint32_t DeleteSubtree(
        std::uint32_t node, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62c190: allocate, link and rebalance a unique target node.
    [[nodiscard]] static std::uint32_t InsertNode(
        std::uint32_t tree, std::uint32_t child,
        std::uint32_t parent, std::uint32_t input,
        IDialogTargetTreeRuntime& runtime);
    // IDA 0x62c400: return the minimum node below a child link.
    [[nodiscard]] static std::uint32_t MinimumFromLink(
        std::uint32_t link, std::uint32_t nil,
        IArenaSequenceRuntime& runtime);
    // IDA 0x62c480: first target whose name is not less than input.
    [[nodiscard]] static std::uint32_t LowerBound(
        std::uint32_t tree, std::uint32_t input,
        IDialogTargetTreeRuntime& runtime);
    // IDA 0x62c510: promote the right child and relink its moved subtree.
    [[nodiscard]] static std::uint32_t PromoteRightChild(
        std::uint32_t tree, std::uint32_t node,
        IDialogTargetTreeRuntime& runtime);
    // IDA 0x62c570: promote the left child and relink its moved subtree.
    [[nodiscard]] static std::uint32_t PromoteLeftChild(
        std::uint32_t tree, std::uint32_t node,
        IDialogTargetTreeRuntime& runtime);
    // IDA 0x62c5d0: allocate a 44-byte node with parent and color.
    [[nodiscard]] static std::uint32_t AllocateNode(
        std::uint32_t parent, std::uint32_t color,
        IDialogTargetTreeRuntime& runtime);
    // IDA 0x62c5f0: move to the previous in-order target node.
    [[nodiscard]] static std::uint32_t Previous(
        std::uint32_t node, IDialogTargetTreeRuntime& runtime);
};

}  // namespace pal4
