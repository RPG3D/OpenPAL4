#include "pal4/dialog_text_tree.h"

#include "pal4/arena_sequence_resources.h"
#include "pal4/monster_manager_tree_rotation.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {

MonsterManagerTreeInsertResult DialogTextTree::Insert(
    std::uint32_t tree, std::uint32_t input,
    IDialogTextTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, input, runtime);
}

void DialogTextTree::EraseNode(std::uint32_t tree, std::uint32_t result,
                               std::uint32_t node, std::uint32_t nil,
                               IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime);
}

void DialogTextTree::EraseRange(std::uint32_t tree, std::uint32_t result,
                                std::uint32_t first, std::uint32_t last,
                                std::uint32_t nil,
                                IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseRange(tree, result, first, last, nil, runtime);
}

std::uint32_t DialogTextTree::LowerBound(
    std::uint32_t tree, std::uint32_t input,
    IDialogTextTreeRuntime& runtime) {
    const auto head = runtime.ReadDword(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadDword(head + 4u);
    while (node != runtime.SharedNil()) {
        if (runtime.CompareText(node + 12u, input) >= 0) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t DialogTextTree::DeleteSubtree(
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(node, nil, runtime);
}

std::uint32_t DialogTextTree::InsertNode(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t input,
    IDialogTextTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, input, runtime);
}

std::uint32_t DialogTextTree::PromoteRightChild(
    std::uint32_t tree, std::uint32_t node,
    IDialogTextTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t DialogTextTree::MinimumFromLink(
    std::uint32_t link, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::MinimumFromLink(link, nil, runtime);
}

std::uint32_t DialogTextTree::PromoteLeftChild(
    std::uint32_t tree, std::uint32_t node,
    IDialogTextTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

std::uint32_t DialogTextTree::Previous(
    std::uint32_t node, IDialogTextTreeRuntime& runtime) {
    return MonsterManagerTreeCursor::Predecessor(
        node, runtime.SharedNil(), [&](std::uint32_t address) {
            return runtime.ReadDword(address);
        }, runtime.ColorOffset()).next;
}

std::uint32_t DialogTextTree::Next(
    std::uint32_t node, IDialogTextTreeRuntime& runtime) {
    return MonsterManagerTreeCursor::Successor(
        node, runtime.SharedNil(), [&](std::uint32_t address) {
            return runtime.ReadDword(address);
        }).next;
}

}  // namespace pal4
