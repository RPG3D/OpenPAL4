#include "pal4/effect_system_factory_text_tree.h"
#include "pal4/monster_manager_tree_cursor.h"
#include "pal4/monster_manager_tree_rotation.h"
#include "pal4/ui_text_buffer_record.h"

namespace pal4 {

std::int32_t IEffectSystemFactoryTextInsertRuntime::CompareRange(
    std::uint32_t input, std::uint32_t node_text) {
    return CompareNames(input, node_text);
}

std::int32_t IEffectSystemFactoryTextInsertRuntime::CompareText(
    std::uint32_t first, std::uint32_t second) {
    return CompareNames(first, second);
}

void IEffectSystemFactoryTextInsertRuntime::AssignText(
    std::uint32_t destination, std::uint32_t source) {
    CopyFactoryRecord(destination, source);
}

void IEffectSystemFactoryTextInsertRuntime::SetNodeData(
    MonsterManagerTreeInsertResult& result,
    std::uint32_t candidate) {
    result = {candidate, false};
}

EffectFactoryTreeInsertResult EffectSystemFactoryTextTree::SearchAndInsert(
    std::uint32_t tree, EffectFactoryTreeEntry entry,
    IEffectSystemFactoryTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto parent = head;
    auto child = runtime.ReadWord(head + 4u);
    bool went_left = true;
    while (child != kNil) {
        parent = child;
        went_left = runtime.Compare(entry.name, child) < 0;
        child = runtime.ReadWord(child + (went_left ? 0u : 8u));
    }

    if (runtime.ReadByte(tree + 8u) != 0u)
        return {runtime.InsertAt(tree, child, parent, entry), true};

    auto candidate = parent;
    if (went_left) {
        if (parent == runtime.ReadWord(head))
            return {runtime.InsertAt(tree, child, parent, entry), true};
        candidate = runtime.Previous(parent);
    }
    if (runtime.Compare(entry.name, candidate) > 0)
        return {runtime.InsertAt(tree, child, parent, entry), true};
    return {candidate, false};
}

EffectFactoryTreeInsertResult EffectSystemFactoryTextTree::InsertTextNode(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemFactoryTextInsertRuntime& runtime) {
    const auto result = MonsterManagerTreeInsert::Insert(
        tree, record, runtime);
    return {result.node, result.inserted};
}

std::uint32_t EffectSystemFactoryTextTree::InsertAt(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectSystemFactoryTextInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

std::uint32_t EffectSystemFactoryTextTree::RotateLeft(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemFactoryTextInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t EffectSystemFactoryTextTree::RotateRight(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemFactoryTextInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

std::uint32_t EffectSystemFactoryTextTree::Clear(
    std::uint32_t tree,
    IEffectSystemFactoryTextLifecycleRuntime& runtime) {
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
        node = runtime.Next(node);
        runtime.EraseNode(tree, current);
    }
    return node;
}

std::uint32_t EffectSystemFactoryTextTree::FindExact(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemFactoryTextLookupRuntime& runtime) {
    const auto candidate = LowerBound(tree, record, runtime);
    const auto head = runtime.ReadWord(tree + 4u);
    return candidate != head && runtime.CompareText(candidate + 12u, record) == 0
               ? candidate : head;
}

std::uint32_t EffectSystemFactoryTextTree::LowerBound(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemFactoryTextLookupRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    for (auto node = runtime.ReadWord(head + 4u); node != kNil;) {
        if (runtime.CompareText(node + 12u, record) >= 0) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

EffectFactoryTreeStep EffectSystemFactoryTextTree::Next(
    std::uint32_t node,
    IEffectSystemFactoryTextCursorRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Successor(
        node, kNil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    return {step.next, step.result};
}

void EffectSystemFactoryTextTree::CopyTextBuffer(
    std::uint32_t destination, std::uint32_t source,
    IUiTextBufferRecordRuntime& runtime) {
    UiTextBufferRecord::InitializeAndCopy(destination, source, runtime);
}

}  // namespace pal4
