#include "pal4/effect_system_task_factory_tree.h"
#include "pal4/monster_manager_tree_rotation.h"
#include "pal4/ui_text_buffer_record.h"

namespace pal4 {

std::int32_t IEffectSystemTaskFactoryTreeRuntime::CompareRange(
    std::uint32_t input, std::uint32_t node_text) {
    return CompareNames(input, node_text);
}

std::int32_t IEffectSystemTaskFactoryTreeRuntime::CompareText(
    std::uint32_t first, std::uint32_t second) {
    return CompareNames(first, second);
}

void IEffectSystemTaskFactoryTreeRuntime::AssignText(
    std::uint32_t destination, std::uint32_t source) {
    CopyFactoryRecord(destination, source);
}

void IEffectSystemTaskFactoryTreeRuntime::SetNodeData(
    MonsterManagerTreeInsertResult& result, std::uint32_t candidate) {
    result = {candidate, false};
}

bool EffectSystemTaskFactoryTree::ProcessEvent(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    const auto head = runtime.ReadDword(tree + 4u);
    if (FindExact(tree, record, runtime) != head) return false;
    return InsertTextNode(tree, record, runtime).inserted;
}

MonsterManagerTreeInsertResult EffectSystemTaskFactoryTree::SearchAndInsert(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, record, runtime);
}

std::uint32_t EffectSystemTaskFactoryTree::FindExact(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    const auto head = runtime.ReadDword(tree + 4u);
    const auto candidate = LowerBound(tree, record, runtime);
    return candidate != head &&
                   runtime.CompareNames(candidate + 12u, record) == 0
               ? candidate : head;
}

std::uint32_t EffectSystemTaskFactoryTree::LowerBound(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    const auto head = runtime.ReadDword(tree + 4u);
    auto candidate = head;
    for (auto node = runtime.ReadDword(head + 4u); node != kNil;) {
        if (runtime.CompareNames(node + 12u, record) >= 0) {
            candidate = node;
            node = runtime.ReadDword(node);
        } else {
            node = runtime.ReadDword(node + 8u);
        }
    }
    return candidate;
}

MonsterManagerTreeInsertResult EffectSystemTaskFactoryTree::InsertTextNode(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, record, runtime);
}

std::uint32_t EffectSystemTaskFactoryTree::Clear(
    std::uint32_t tree,
    IEffectSystemTaskFactoryTreeLifecycleRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        node = runtime.ReadWord(head + 4u);
        while (node != kNil) {
            runtime.DeleteSubtree(runtime.ReadWord(node + 8u));
            const auto left = runtime.ReadWord(node);
            runtime.ReleaseText(node + 12u);
            runtime.FreeNode(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, kNil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        return head;
    }
    while (node != head) {
        const auto current = node;
        node = runtime.Advance(node);
        runtime.EraseNode(tree, current);
    }
    return node;
}

std::uint32_t EffectSystemTaskFactoryTree::InsertAt(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

std::uint32_t EffectSystemTaskFactoryTree::RotateLeft(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t EffectSystemTaskFactoryTree::RotateRight(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemTaskFactoryTreeRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

MonsterManagerTreeStep EffectSystemTaskFactoryTree::Previous(
    std::uint32_t node, IEffectSystemTaskFactoryTreeRuntime& runtime) {
    return MonsterManagerTreeCursor::Predecessor(
        node, kNil,
        [&](std::uint32_t address) { return runtime.ReadDword(address); },
        32u);
}

std::uint32_t EffectSystemTaskFactoryTree::CopyRecord(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeAndCopy(destination, source, runtime);
    return destination;
}

void EffectSystemTaskFactoryTree::CopyRecordIfPresent(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    if (destination != 0u)
        (void)CopyRecord(destination, source, runtime);
}

}  // namespace pal4
