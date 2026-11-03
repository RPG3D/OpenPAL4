#include "pal4/dialog_target_tree.h"

#include "pal4/arena_sequence_resources.h"
#include "pal4/monster_manager_tree_rotation.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {

MonsterManagerTreeInsertResult DialogTargetTree::Insert(
    std::uint32_t tree, std::uint32_t input,
    IDialogTargetTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, input, runtime);
}

namespace {

template <class Read>
std::uint32_t Successor(std::uint32_t node, std::uint32_t nil,
                        Read&& read) {
    auto right = read(node + 8u);
    if (right != nil) {
        while (read(right) != nil)
            right = read(right);
        return right;
    }
    auto parent = read(node + 4u);
    while (node == read(parent + 8u)) {
        node = parent;
        parent = read(parent + 4u);
    }
    return read(node + 8u) != parent ? parent : node;
}

}  // namespace

void DialogTargetTree::Advance(std::uint32_t& iterator,
                               IDialogTargetTreeCursorRuntime& runtime) {
    iterator = Successor(iterator, runtime.SharedNil(),
                         [&](std::uint32_t address) {
                             return runtime.ReadWord(address);
                         });
}

void DialogTargetTree::EraseNode(std::uint32_t tree, std::uint32_t result,
                                 std::uint32_t node, std::uint32_t nil,
                                 IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime, 40u);
}

std::uint32_t DialogTargetTree::DeleteSubtree(
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(node, nil, runtime);
}

std::uint32_t DialogTargetTree::InsertNode(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t input,
    IDialogTargetTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, input, runtime);
}

std::uint32_t DialogTargetTree::MinimumFromLink(
    std::uint32_t link, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::MinimumFromLink(link, nil, runtime);
}

std::uint32_t DialogTargetTree::LowerBound(
    std::uint32_t tree, std::uint32_t input,
    IDialogTargetTreeRuntime& runtime) {
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

std::uint32_t DialogTargetTree::PromoteRightChild(
    std::uint32_t tree, std::uint32_t node,
    IDialogTargetTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t DialogTargetTree::PromoteLeftChild(
    std::uint32_t tree, std::uint32_t node,
    IDialogTargetTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

std::uint32_t DialogTargetTree::AllocateNode(
    std::uint32_t parent, std::uint32_t color,
    IDialogTargetTreeRuntime& runtime) {
    const auto node = runtime.AllocateMemory(44u);
    runtime.WriteDword(node + 4u, parent);
    runtime.WriteDword(node + 40u, color);
    return node;
}

std::uint32_t DialogTargetTree::Previous(
    std::uint32_t node, IDialogTargetTreeRuntime& runtime) {
    return MonsterManagerTreeCursor::Predecessor(
        node, runtime.SharedNil(), [&](std::uint32_t address) {
            return runtime.ReadDword(address);
        }, runtime.ColorOffset()).next;
}

std::uint32_t DialogTargetTree::EraseRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u &&
        first == runtime.ReadWord(head) && last == head) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != nil) {
            (void)DeleteSubtree(runtime.ReadWord(node + 8u), nil, runtime);
            const auto left = runtime.ReadWord(node);
            ArenaSequenceResources::ReleaseText(node + 12u, runtime);
            (void)runtime.Free(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, nil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.WriteWord(result, head);
        return head;
    }

    auto node = first;
    while (node != last) {
        const auto current = node;
        node = Successor(node, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
        EraseNode(tree, result, current, nil, runtime);
    }
    runtime.WriteWord(result, node);
    return node;
}

}  // namespace pal4
