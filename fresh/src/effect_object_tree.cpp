#include "pal4/effect_object_tree.h"

#include "pal4/arena_sequence_resources.h"
#include "pal4/monster_manager_tree_cursor.h"
#include "pal4/monster_manager_tree_rotation.h"
#include "pal4/ui_text_buffer_record.h"

namespace pal4 {

std::uint32_t EffectObjectTree::AdvanceAndCapture(
    std::uint32_t& iterator, std::uint32_t result,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto previous = iterator;
    const auto step = MonsterManagerTreeCursor::Successor(
        iterator, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    iterator = step.next;
    runtime.WriteWord(result, previous);
    return result;
}

void EffectObjectRecord::CopyTextAndValue(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeAndCopy(destination, source, runtime);
}

MonsterManagerTreeInsertResult EffectObjectTree::FindOrCreate(
    std::uint32_t tree, std::uint32_t record,
    IEffectObjectTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, record, runtime);
}

void EffectObjectTree::RemoveNode(
    std::uint32_t tree, std::uint32_t result, std::uint32_t node,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime, 32u);
}

std::uint32_t EffectObjectTree::FindExact(
    std::uint32_t tree, std::uint32_t key,
    IEffectObjectTreeRuntime& runtime) {
    const auto head = runtime.ReadDword(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadDword(head + 4u);
    while (node != runtime.SharedNil()) {
        if (runtime.CompareText(node + 12u, key) >= 0) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    }
    return candidate != head &&
                   runtime.CompareText(key, candidate + 12u) >= 0
               ? candidate : head;
}

std::uint32_t EffectObjectTree::Advance(
    std::uint32_t& iterator, IEffectObjectTreeRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Successor(
        iterator, runtime.SharedNil(), [&](std::uint32_t address) {
            return runtime.ReadDword(address);
        });
    iterator = step.next;
    return step.result;
}

std::uint32_t EffectObjectTree::Previous(
    std::uint32_t& iterator, IEffectObjectTreeRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Predecessor(
        iterator, runtime.SharedNil(), [&](std::uint32_t address) {
            return runtime.ReadDword(address);
        });
    iterator = step.next;
    return step.result;
}

std::uint32_t EffectObjectTree::DeleteSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime);
}

std::uint32_t EffectObjectTree::InsertNode(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectObjectTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

std::uint32_t EffectObjectTree::PromoteRightChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectObjectTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t EffectObjectTree::PromoteLeftChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectObjectTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

}  // namespace pal4
