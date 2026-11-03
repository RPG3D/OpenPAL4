#include "pal4/effect_system.h"

#include "pal4/arena_sequence_resources.h"
#include "pal4/monster_manager_tree_cursor.h"
#include "pal4/monster_manager_tree_rotation.h"

#include <array>
#include <string>
#include <utility>

namespace pal4 {
namespace {

struct QueueSelection {
    std::uint32_t offset;
    bool inline_effect;
};

QueueSelection SelectQueue(
    std::uint32_t object,
    IEffectSystemObjectClassifyRuntime& runtime) {
    const auto type = runtime.ObjectTypeName(object);
    if (runtime.UseFallbackQueue(object)) return {200u, false};
    if (runtime.UseCameraQueue(object)) return {152u, false};
    if (type == "EffectObject") return {212u, true};

    constexpr std::array<std::pair<std::string_view, std::uint32_t>, 10u>
        queues{{
            {"ClumpObject", 164u},
            {"RtkPrtStdAtomicObject", 176u},
            {"Sound3DObject", 188u},
            {"CylinderObject", 140u},
            {"StripObject", 104u},
            {"BillBordObject", 116u},
            {"CameraObject", 68u},
            {"SequnceAnimObject", 128u},
            {"DynamicStripObject", 80u},
            {"ThunderObject", 92u},
        }};
    for (const auto& [name, offset] : queues)
        if (type == name) return {offset, false};
    return {200u, false};
}

std::uint32_t CleanupEmbeddedTree(
    std::uint32_t tree, EffectSystemEmbeddedTreeKind kind,
    IEffectSystemEmbeddedTreeRuntime& runtime,
    bool read_first_element = true) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.DeleteSubtree(kind, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil(kind));
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        if (read_first_element) runtime.ReadFirstElement(tree);
    } else {
        while (node != head) {
            const auto next = runtime.Next(kind, node);
            runtime.RemoveNode(kind, tree, node);
            node = next;
        }
    }

    (void)runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto references = runtime.ReleaseSharedNilReference(kind);
    if (references != 0u) return references;
    const auto nil = runtime.SharedNil(kind);
    runtime.ClearSharedNil(kind);
    return nil != 0u ? runtime.Free(nil) : 0u;
}

std::uint32_t InitializeEmbeddedTreeStorage(
    std::uint32_t tree, EffectSystemEmbeddedTreeKind kind,
    std::uint32_t node_size, std::uint32_t color_offset,
    IEffectSystemTreeStorageRuntime& runtime) {
    const auto candidate = runtime.Allocate(node_size);
    runtime.WriteWord(candidate + color_offset, 1u);
    runtime.WriteWord(candidate + 4u, 0u);
    const bool created_nil = runtime.SharedNil(kind) == 0u;
    if (created_nil) {
        runtime.SetSharedNil(kind, candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    }
    runtime.SetSharedNilReferences(
        kind, runtime.SharedNilReferences(kind) + 1u);
    if (!created_nil) runtime.Free(candidate);
    const auto head = runtime.Allocate(node_size);
    runtime.WriteWord(head + 4u, runtime.SharedNil(kind));
    runtime.WriteWord(head + color_offset, 0u);
    runtime.WriteWord(tree + 4u, head);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return head;
}

}  // namespace

std::uint32_t EffectSystemEmbeddedTrees::LowerBoundFirst(
    std::uint32_t tree, std::uint32_t key,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != nil) {
        if (runtime.ReadWord(node + 12u) >= key) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t EffectSystemEmbeddedTrees::FindFirstExact(
    std::uint32_t tree, std::uint32_t key,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    const auto candidate = LowerBoundFirst(tree, key, nil, runtime);
    return candidate != head && runtime.ReadWord(candidate + 12u) == key
               ? candidate : head;
}

void EffectSystemEmbeddedTrees::LowerBoundFirstInto(
    std::uint32_t tree, std::uint32_t& result, std::uint32_t key,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    result = LowerBoundFirst(tree, key, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::InsertSecondAt(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::PreviousSecond(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return PreviousThird(iterator, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::LowerBoundFourth(
    std::uint32_t tree, std::uint32_t key,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    return LowerBoundFirst(tree, key, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::DeleteFirstSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeFirstStorage(
    std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime) {
    return InitializeEmbeddedTreeStorage(
        tree, EffectSystemEmbeddedTreeKind::First, 28u, 24u, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::InsertFirstAt(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectSystemFirstTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::PromoteFirstRightChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemFirstTreeInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::FirstMinimumFromLink(
    std::uint32_t link, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::MinimumFromLink(link, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::PromoteFirstLeftChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemFirstTreeInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::DeleteSecondSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeSecondStorage(
    std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime) {
    return InitializeEmbeddedTreeStorage(
        tree, EffectSystemEmbeddedTreeKind::Second, 24u, 20u, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::SecondMinimumFromLink(
    std::uint32_t link, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::MinimumFromLink(link, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::DeleteThirdSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeThirdStorage(
    std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime) {
    return InitializeEmbeddedTreeStorage(
        tree, EffectSystemEmbeddedTreeKind::Third, 24u, 20u, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::InsertThirdAt(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::PromoteThirdRightChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::ThirdMinimumFromLink(
    std::uint32_t link, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::MinimumFromLink(link, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::PromoteThirdLeftChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

void EffectSystemEmbeddedTrees::EraseFourthNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime,
                                 20u, false);
}

std::uint32_t EffectSystemEmbeddedTrees::DeleteFourthSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeFourthStorage(
    std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime) {
    return InitializeEmbeddedTreeStorage(
        tree, EffectSystemEmbeddedTreeKind::Fourth, 24u, 20u, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::InsertFourthAt(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::DeleteFifthSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeFifthStorage(
    std::uint32_t tree, IEffectSystemTreeStorageRuntime& runtime) {
    return InitializeEmbeddedTreeStorage(
        tree, EffectSystemEmbeddedTreeKind::Fifth, 24u, 20u, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::InsertFifthAt(
    std::uint32_t tree, std::uint32_t child,
    std::uint32_t parent, std::uint32_t record,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::InsertHelper(
        tree, child, parent, record, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::CreateFirstNode(
    std::uint32_t parent, std::uint32_t color,
    IEffectSystemTreeStorageRuntime& runtime) {
    const auto node = runtime.Allocate(28u);
    runtime.WriteWord(node + 4u, parent);
    runtime.WriteWord(node + 24u, color);
    return node;
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::PreviousFirst(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Predecessor(
        iterator, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }, 24u);
    iterator = step.next;
    return step;
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::PreviousThird(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Predecessor(
        iterator, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        }, 20u);
    iterator = step.next;
    return step;
}

std::uint32_t EffectSystemEmbeddedTrees::PromoteFourthRightChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateRight(tree, node, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::PromoteFourthLeftChild(
    std::uint32_t tree, std::uint32_t node,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeRotation::RotateLeft(tree, node, runtime);
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::PreviousFourth(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return PreviousThird(iterator, nil, runtime);
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::PreviousFifth(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return PreviousThird(iterator, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupFirst(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::First,
                               runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupSecond(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Second,
                               runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupThird(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Third,
                               runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupFourth(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Fourth,
                               runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupFifth(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Fifth,
                               runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupFirstDirect(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::First,
                               runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupSecondDirect(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Second,
                               runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupThirdDirect(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Third,
                               runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupFourthDirect(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Fourth,
                               runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::CleanupFifthDirect(
    std::uint32_t tree, IEffectSystemEmbeddedTreeRuntime& runtime) {
    return CleanupEmbeddedTree(tree, EffectSystemEmbeddedTreeKind::Fifth,
                               runtime, false);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeFirst(
    std::uint32_t tree, std::uint8_t first_flag,
    std::uint8_t second_flag, std::uint8_t duplicate_mode,
    IEffectSystemEmbeddedTreeConstructRuntime& runtime) {
    runtime.WriteByte(tree, first_flag);
    runtime.WriteByte(tree + 8u, duplicate_mode);
    runtime.WriteByte(tree + 1u, second_flag);
    runtime.InitializeTree(tree);
    return tree;
}

std::int32_t IEffectSystemFirstTreeInsertRuntime::CompareRange(
    std::uint32_t input, std::uint32_t node_data) {
    return CompareText(input, node_data);
}

std::int32_t IEffectSystemFirstTreeInsertRuntime::CompareText(
    std::uint32_t first, std::uint32_t second) {
    const auto lhs = ReadDword(first);
    const auto rhs = ReadDword(second);
    return lhs < rhs ? -1 : (lhs > rhs ? 1 : 0);
}

void IEffectSystemFirstTreeInsertRuntime::AssignText(
    std::uint32_t destination, std::uint32_t source) {
    for (std::uint32_t offset = 0u; offset < 12u; offset += 4u)
        WriteDword(destination + offset, ReadDword(source + offset));
}

void IEffectSystemFirstTreeInsertRuntime::SetNodeData(
    MonsterManagerTreeInsertResult& result, std::uint32_t candidate) {
    result.node = candidate;
}

MonsterManagerTreeInsertResult EffectSystemEmbeddedTrees::InsertFirst(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemFirstTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, record, runtime);
}

void EffectSystemEmbeddedTrees::EraseFirstNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime,
                                 24u, false);
}

void EffectSystemEmbeddedTrees::EraseFirstRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseRange(tree, result, first, last, nil,
                                  runtime, 24u, false);
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::AdvanceFirst(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Successor(
        iterator, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    iterator = step.next;
    return step;
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeSecond(
    std::uint32_t tree, std::uint8_t first_flag,
    std::uint8_t second_flag, std::uint8_t duplicate_mode,
    IEffectSystemEmbeddedTreeConstructRuntime& runtime) {
    return InitializeFirst(tree, first_flag, second_flag,
                           duplicate_mode, runtime);
}

void EffectSystemEmbeddedTrees::EraseSecondNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime,
                                 20u, false);
}

void EffectSystemEmbeddedTrees::EraseSecondRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseRange(tree, result, first, last, nil,
                                  runtime, 20u, false);
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::AdvanceSecond(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return AdvanceFirst(iterator, nil, runtime);
}

std::int32_t IEffectSystemThirdTreeInsertRuntime::CompareRange(
    std::uint32_t input, std::uint32_t node_data) {
    return CompareText(input, node_data);
}

std::int32_t IEffectSystemThirdTreeInsertRuntime::CompareText(
    std::uint32_t first, std::uint32_t second) {
    const auto lhs = ReadDword(first);
    const auto rhs = ReadDword(second);
    return lhs < rhs ? -1 : (lhs > rhs ? 1 : 0);
}

void IEffectSystemThirdTreeInsertRuntime::AssignText(
    std::uint32_t destination, std::uint32_t source) {
    WriteDword(destination, ReadDword(source));
    WriteDword(destination + 4u, ReadDword(source + 4u));
}

void IEffectSystemThirdTreeInsertRuntime::SetNodeData(
    MonsterManagerTreeInsertResult& result, std::uint32_t candidate) {
    result.node = candidate;
}

MonsterManagerTreeInsertResult EffectSystemEmbeddedTrees::InsertThird(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, record, runtime);
}

void EffectSystemEmbeddedTrees::EraseThirdNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    EraseSecondNode(tree, result, node, nil, runtime);
}

void EffectSystemEmbeddedTrees::EraseThirdRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    EraseSecondRange(tree, result, first, last, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::FindThird(
    std::uint32_t tree, std::uint32_t key,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != nil) {
        if (runtime.ReadWord(node + 12u) >= key) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate != head && runtime.ReadWord(candidate + 12u) == key
        ? candidate : head;
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::AdvanceThird(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return AdvanceFirst(iterator, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeThird(
    std::uint32_t tree, std::uint8_t first_flag,
    std::uint8_t second_flag, std::uint8_t duplicate_mode,
    IEffectSystemEmbeddedTreeConstructRuntime& runtime) {
    return InitializeFirst(tree, first_flag, second_flag,
                           duplicate_mode, runtime);
}

MonsterManagerTreeInsertResult EffectSystemEmbeddedTrees::InsertFourth(
    std::uint32_t tree, std::uint32_t record,
    IEffectSystemThirdTreeInsertRuntime& runtime) {
    return MonsterManagerTreeInsert::Insert(tree, record, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::ClearFourth(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t nil, IArenaSequenceRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    ArenaSequenceTree::EraseRange(tree, result, runtime.ReadWord(head),
                                  head, nil, runtime, 20u, false);
    return runtime.ReadWord(result);
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::AdvanceFourth(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return AdvanceFirst(iterator, nil, runtime);
}

std::uint32_t EffectSystemEmbeddedTrees::InitializeFifth(
    std::uint32_t tree, std::uint8_t first_flag,
    std::uint8_t second_flag, std::uint8_t duplicate_mode,
    IEffectSystemEmbeddedTreeConstructRuntime& runtime) {
    return InitializeFirst(tree, first_flag, second_flag,
                           duplicate_mode, runtime);
}

void EffectSystemEmbeddedTrees::EraseFifthNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    EraseSecondNode(tree, result, node, nil, runtime);
}

MonsterManagerTreeStep EffectSystemEmbeddedTrees::AdvanceFifth(
    std::uint32_t& iterator, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return AdvanceFirst(iterator, nil, runtime);
}

std::uint32_t EffectSystem::Initialize(
    std::uint32_t system, IEffectSystemInitRuntime& runtime) {
    runtime.WriteByte(system + 4u, 0u);
    runtime.WriteWord(system + 8u, 0u);
    runtime.InitializeFirstTree(system + 12u);
    const auto state = runtime.InitializeSecondTree(system + 28u);
    runtime.WriteWord(system + 48u, 0u);
    runtime.WriteByte(system + 52u, state);
    for (const auto offset : {56u, 60u, 64u})
        runtime.WriteWord(system + offset, 0u);

    for (std::uint32_t offset = 68u; offset <= 200u; offset += 12u) {
        runtime.WriteByte(system + offset, state);
        runtime.WriteWord(system + offset + 4u, runtime.CreateListHead());
        runtime.WriteWord(system + offset + 8u, 0u);
    }
    runtime.WriteByte(system + 212u, state);
    runtime.WriteByte(system + 213u, state);
    runtime.WriteByte(system + 220u, 0u);
    runtime.InitializeInlineState(system + 212u);
    runtime.InitializeFourthTree(system + 228u);
    runtime.InitializeFifthTree(system + 244u);
    runtime.WriteWord(system, 0x84b19cu);
    return system;
}

std::uint32_t EffectSystem::Destroy(
    std::uint32_t system, IEffectSystemDestroyRuntime& runtime) {
    runtime.WriteWord(system, 0x84b19cu);
    Disable(system, runtime);
    for (std::uint32_t offset = 244u; offset >= 212u; offset -= 16u)
        runtime.CleanupTrailingTree(system + offset);

    for (std::uint32_t offset = 200u; offset >= 116u; offset -= 12u) {
        const auto list = system + offset;
        const auto head = runtime.ReadWord(list + 4u);
        const auto first = offset >= 176u
            ? runtime.ReadWord(head)
            : runtime.FirstListElement(list);
        runtime.RemoveListElements(list, first, head);
        runtime.Free(head);
        runtime.WriteWord(list + 4u, 0u);
        runtime.WriteWord(list + 8u, 0u);
    }
    for (std::uint32_t offset = 104u; offset >= 68u; offset -= 12u)
        runtime.CleanupConfigList(system + offset);

    const auto range = runtime.ReadWord(system + 56u);
    runtime.ReleaseRange(range, runtime.ReadWord(system + 60u));
    runtime.Free(range);
    for (std::uint32_t offset = 56u; offset <= 64u; offset += 4u)
        runtime.WriteWord(system + offset, 0u);
    runtime.CleanupSecondTree(system + 28u);
    return runtime.CleanupFirstTree(system + 12u);
}

std::uint32_t EffectSystem::TotalCount(
    std::uint32_t system, IEffectSystemCountRuntime& runtime) {
    std::uint32_t total = 0u;
    for (std::uint32_t offset = 76u; offset <= 208u; offset += 12u)
        total += runtime.ReadWord(system + offset);
    return total + runtime.ReadWord(system + 224u);
}

std::uint8_t EffectSystem::Enable(
    std::uint32_t system, std::uint32_t effect,
    std::uint32_t ignored, IEffectSystemEnableRuntime& runtime) {
    (void)ignored;
    Disable(system, runtime);
    if (effect == 0u) return 0u;
    const auto manager = runtime.InitializeSystem();
    if (runtime.ReadByte(manager) == 0u) return 0u;

    runtime.WriteByte(system + 4u, 1u);
    runtime.WriteWord(system + 48u, effect);
    runtime.WriteWord(system + 44u, 0x3f800000u);
    runtime.InitializeGeometryData(system + 52u);
    runtime.AddEffect(runtime.InitializeSystem(), system);
    return 1u;
}

void EffectSystem::Disable(
    std::uint32_t system, IEffectSystemDisableRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u) return;
    runtime.CleanupEffects(system);
    runtime.InitializeGeometryData(system + 52u);
    runtime.WriteByte(system + 4u, 0u);
    runtime.WriteWord(system + 48u, 0u);

    const auto inline_list = system + 212u;
    const auto inline_head = runtime.ReadWord(inline_list + 4u);
    runtime.RemoveInlineEffects(
        inline_list, runtime.FirstListElement(inline_list), inline_head);

    constexpr std::uint32_t offsets[] = {
        68u, 92u, 80u, 104u, 116u, 128u, 140u,
        152u, 164u, 176u, 188u, 200u};
    for (const auto offset : offsets) {
        const auto list = system + offset;
        const auto head = runtime.ReadWord(list + 4u);
        const auto first = offset <= 140u
            ? runtime.ReadWord(head)
            : runtime.FirstListElement(list);
        runtime.RemoveListElements(list, first, head);
    }
    runtime.RemoveEffect(runtime.InitializeSystem(), system);
}

int EffectSystem::UpdateFrame(
    std::uint32_t system, double seconds,
    IEffectSystemFrameRuntime& runtime) {
    const auto enabled = runtime.ReadByte(system + 4u);
    if (enabled == 0u) return 0;

    const double scaled = static_cast<double>(
        runtime.ReadFloat(system + 44u)) * seconds;
    const double rounded = static_cast<double>(
        static_cast<float>(scaled));
    runtime.UpdateProperties2(scaled);
    runtime.UpdateTimers2(system, rounded);
    runtime.UpdateProperties12(system, rounded);
    runtime.UpdateProperties10(system, rounded);
    runtime.UpdateProperties11(system, rounded);
    return runtime.UpdateTimers(system, rounded);
}

std::uint8_t EffectSystem::Draw(
    std::uint32_t system, std::uint32_t context,
    IEffectSystemDrawRuntime& runtime) {
    const auto enabled = runtime.ReadByte(system + 4u);
    if (enabled == 0u) return enabled;

    constexpr std::uint32_t states[] = {6u, 8u, 12u, 10u, 11u};
    constexpr std::uint32_t values[] = {1u, 1u, 1u, 5u, 6u};
    for (std::uint32_t index = 0u; index < 5u; ++index)
        (void)runtime.SetRenderState(states[index], values[index]);

    const auto prior_state = runtime.GetDeviceState28();
    runtime.SetDeviceState28(0u);
    runtime.DrawProperties13(system, context);
    runtime.DrawProperties15(system, context);
    runtime.DrawProperties(system, context);
    runtime.DrawProperties16(system, context);
    runtime.DrawProperties14(system, context);
    runtime.SetDeviceState28(prior_state);

    int result = 0;
    for (std::uint32_t index = 0u; index < 5u; ++index)
        result = runtime.SetRenderState(states[index], values[index]);
    return static_cast<std::uint8_t>(result);
}

std::uint8_t EffectSystem::ProcessClumpAtomics(
    std::uint32_t system, std::uint32_t clump,
    IEffectSystemClumpRuntime& runtime) {
    const auto enabled = runtime.ReadByte(system + 4u);
    return enabled != 0u
        ? runtime.ProcessAtomics(clump, system + 52u) : enabled;
}

std::uint8_t EffectSystem::ProcessClumpAtomicsWrapper(
    std::uint32_t system, std::uint32_t clump,
    IEffectSystemClumpRuntime& runtime) {
    const auto enabled = runtime.ReadByte(system + 4u);
    return enabled != 0u
        ? runtime.ProcessAtomicsWrapper(clump) : enabled;
}

std::uint8_t EffectSystem::QueueObject(
    std::uint32_t system, std::uint32_t object,
    IEffectSystemObjectQueueRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u || object == 0u) return 0u;

    const auto selection = SelectQueue(object, runtime);
    if (selection.inline_effect) {
        runtime.AddInlineEffect(system + 212u, object, 1u);
        return 1u;
    }
    const auto list = system + selection.offset;
    runtime.AddToList(list, runtime.ReadWord(list + 4u), object);
    return 1u;
}

bool EffectSystem::RemoveObject(
    std::uint32_t system, std::uint32_t object,
    IEffectSystemObjectRemoveRuntime& runtime) {
    if (runtime.ReadByte(system + 4u) == 0u || object == 0u) return false;

    const auto selection = SelectQueue(object, runtime);
    if (selection.inline_effect) {
        const auto tree = system + 212u;
        const auto node = runtime.FindInlineEffect(tree, object);
        if (node == runtime.ReadWord(tree + 4u)) return false;
        runtime.WriteByte(node + 16u, 0u);
        return true;
    }

    const auto list = system + selection.offset;
    const auto head = runtime.ReadWord(list + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        if (runtime.ReadWord(node + 8u) == object) {
            const auto previous = runtime.ReadWord(node + 4u);
            const auto next = runtime.ReadWord(node);
            runtime.WriteWord(previous, next);
            runtime.WriteWord(next + 4u, previous);
            runtime.FreeNode(node);
            runtime.WriteWord(list + 8u,
                              runtime.ReadWord(list + 8u) - 1u);
            return true;
        }
        node = runtime.ReadWord(node);
    }
    return false;
}

std::uint32_t EffectSystemTree::LowerBound(
    std::uint32_t tree, std::uint32_t key,
    IEffectSystemTreeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto candidate = head;
    auto node = runtime.ReadWord(head + 4u);
    while (node != runtime.SharedNil()) {
        if (runtime.CompareText(node + 12u, key) >= 0) {
            candidate = node;
            node = runtime.ReadWord(node);
        } else {
            node = runtime.ReadWord(node + 8u);
        }
    }
    return candidate;
}

std::uint32_t EffectSystemTree::LowerBoundByBytes(
    std::uint32_t tree, std::uint32_t key,
    IEffectSystemTreeRuntime& runtime) {
    return LowerBound(tree, key, runtime);
}

std::uint32_t EffectSystem::CleanupInstances(
    std::uint32_t system, IEffectSystemCleanupRuntime& runtime) {
    const auto initialized = runtime.InitializeSystem();
    if (runtime.ReadByte(initialized) == 0u) return initialized;

    auto head = runtime.ReadWord(system + 4u);
    auto node = runtime.ReadWord(head);
    while (node != head) {
        const auto xml = runtime.ReadWord(node + 28u);
        if (xml != 0u) {
            runtime.DestroyXmlNode(xml);
            runtime.Free(xml);
        }
        node = runtime.RemoveInstance(system, node);
        head = runtime.ReadWord(system + 4u);
    }
    return head;
}

std::uint32_t EffectSystem::LoadEffectFromFile(
    std::uint32_t system, std::string_view name,
    IEffectSystemLoadRuntime& runtime) {
    const auto initialized = runtime.InitializeSystem();
    if (runtime.ReadByte(initialized) == 0u) return 0u;

    name = runtime.BeginLoadName(name);
    struct NameScope {
        IEffectSystemLoadRuntime& runtime;
        ~NameScope() { runtime.EndLoadName(); }
    } name_scope{runtime};
    const auto head = runtime.ReadWord(system + 4u);
    const auto cached = runtime.FindCached(system, name);
    if (cached != head) {
        const auto instance = runtime.CreateInstance();
        try {
            if (runtime.LoadInstance(instance, runtime.ReadWord(cached + 28u)))
                return instance;
        } catch (...) {
            runtime.DestroyInstance(system, instance);
            throw;
        }
        runtime.DestroyInstance(system, instance);
        return 0u;
    }

    runtime.BeginScriptScope();
    std::uint32_t file = 0u;
    std::uint32_t xml = 0u;
    std::uint32_t adapter = 0u;
    bool scripts_active = true;
    const auto end_scripts = [&] {
        runtime.ReleaseFilePath();
        scripts_active = false;
        runtime.EndScriptScope();
    };
    try {
        const auto path = runtime.BuildFilePath(name);
        file = runtime.OpenFile(path);
        if (file == 0u) {
            end_scripts();
            return 0u;
        }

        xml = runtime.CreateXmlDocument(60u);
        adapter = runtime.CreateFileAdapter(file);
        if (!runtime.ParseXml(adapter, xml)) {
            runtime.CloseFile(file);
            file = 0u;
            if (xml != 0u) {
                runtime.DestroyXmlNode(xml);
                runtime.FreeXml(xml);
                xml = 0u;
            }
            runtime.DestroyFileAdapter(adapter);
            adapter = 0u;
            end_scripts();
            return 0u;
        }

        const auto document = xml;
        runtime.InsertCached(system, name, document);
        xml = 0u;
        runtime.CloseFile(file);
        file = 0u;
        const auto instance = runtime.CreateInstance();
        bool loaded{};
        try { loaded = runtime.LoadInstance(instance, document); }
        catch (...) { runtime.DestroyInstance(system, instance); throw; }
        if (!loaded) runtime.DestroyInstance(system, instance);
        runtime.DestroyFileAdapter(adapter);
        adapter = 0u;
        end_scripts();
        return loaded ? instance : 0u;
    } catch (...) {
        if (file != 0u) runtime.CloseFile(file);
        if (xml != 0u) {
            runtime.DestroyXmlNode(xml);
            runtime.FreeXml(xml);
        }
        if (adapter != 0u) runtime.DestroyFileAdapter(adapter);
        if (scripts_active) end_scripts();
        throw;
    }
}

}  // namespace pal4
