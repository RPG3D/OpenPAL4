#include "pal4/ef_effect_system_tree_lifecycle.h"
#include "pal4/arena_sequence_resources.h"
#include "pal4/monster_manager_tree_cursor.h"

namespace pal4 {
namespace {

std::uint32_t InitializeStorage(
    std::uint32_t tree, EfEffectSystemTreeKind kind,
    std::uint32_t size, std::uint32_t color, bool color_first,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    const auto candidate = runtime.Allocate(size);
    if (color_first) {
        runtime.WriteWord(candidate + color, 1u);
        runtime.WriteWord(candidate + 4u, 0u);
    } else {
        runtime.WriteWord(candidate + 4u, 0u);
        runtime.WriteWord(candidate + color, 1u);
    }
    const bool created_nil = runtime.SharedNil(kind) == 0u;
    if (created_nil) {
        runtime.SetSharedNil(kind, candidate);
        runtime.WriteWord(candidate, 0u);
        runtime.WriteWord(candidate + 8u, 0u);
    }
    runtime.SetSharedNilReferences(
        kind, runtime.SharedNilReferences(kind) + 1u);
    if (!created_nil) (void)runtime.Free(candidate);

    const auto head = runtime.Allocate(size);
    runtime.WriteWord(head + 4u, runtime.SharedNil(kind));
    runtime.WriteWord(head + color, 0u);
    runtime.WriteWord(tree + 4u, head);
    runtime.WriteWord(tree + 12u, 0u);
    runtime.WriteWord(head, head);
    runtime.WriteWord(head + 8u, head);
    return head;
}

std::uint32_t InitializeTree(
    std::uint32_t tree, EfEffectSystemTreeKind kind,
    std::uint8_t first, std::uint8_t second,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    const bool small = kind == EfEffectSystemTreeKind::First;
    runtime.WriteByte(tree, first);
    runtime.WriteByte(tree + 1u, second);
    runtime.WriteByte(tree + 8u,
                      kind == EfEffectSystemTreeKind::EffectObject ? 1u : 0u);
    (void)InitializeStorage(tree, kind, small ? 24u : 36u,
                            small ? 20u : 32u,
                            kind == EfEffectSystemTreeKind::EffectObject,
                            runtime);
    return tree;
}

std::uint32_t CleanupTree(
    std::uint32_t tree, EfEffectSystemTreeKind kind,
    IEfEffectSystemTreeLifecycleRuntime& runtime,
    bool read_first_after_delete = false) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto node = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.DeleteSubtree(kind, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.SharedNil(kind));
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        if (read_first_after_delete) runtime.ReadFirstElement(tree);
    } else {
        while (node != head) {
            const auto next = runtime.Next(kind, node);
            runtime.EraseNode(kind, tree, node);
            node = next;
        }
    }
    (void)runtime.Free(head);
    runtime.WriteWord(tree + 4u, 0u);
    runtime.WriteWord(tree + 12u, 0u);
    const auto references = runtime.SharedNilReferences(kind) - 1u;
    runtime.SetSharedNilReferences(kind, references);
    if (references != 0u) return references;
    const auto nil = runtime.SharedNil(kind);
    runtime.SetSharedNil(kind, 0u);
    return nil != 0u ? runtime.Free(nil) : 0u;
}

std::uint32_t InitializeWrapper(
    std::uint32_t tree, EfEffectSystemTreeWrapperKind kind,
    std::uint8_t first, std::uint8_t second, std::uint8_t mode,
    IEfEffectSystemTreeWrapperRuntime& runtime) {
    runtime.WriteByte(tree, first);
    runtime.WriteByte(tree + 8u, mode);
    runtime.WriteByte(tree + 1u, second);
    runtime.InitializeStorage(kind, tree);
    return tree;
}

std::uint32_t EraseRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    EfEffectSystemRangeKind kind, IEfEffectSystemRangeRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    if (runtime.ReadWord(tree + 12u) != 0u &&
        first == runtime.ReadWord(head) && last == head) {
        auto node = runtime.ReadWord(head + 4u);
        while (node != nil) {
            runtime.DeleteSubtree(kind, runtime.ReadWord(node + 8u));
            const auto left = runtime.ReadWord(node);
            if (kind == EfEffectSystemRangeKind::EffectObject ||
                kind == EfEffectSystemRangeKind::Primary ||
                kind == EfEffectSystemRangeKind::Secondary)
                runtime.ReleaseText(node + 12u);
            (void)runtime.Free(node);
            node = left;
        }
        runtime.WriteWord(head + 4u, nil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        runtime.WriteWord(result, head);
        return result;
    }

    auto node = first;
    while (node != last) {
        const auto current = node;
        node = runtime.Next(kind, node);
        runtime.EraseNode(kind, tree, current);
    }
    runtime.WriteWord(result, node);
    return result;
}

std::uint32_t AdvanceAndCapture(
    std::uint32_t& iterator, std::uint32_t result, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    const auto previous = iterator;
    const auto step = MonsterManagerTreeCursor::Successor(
        iterator, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    iterator = step.next;
    runtime.WriteWord(result, previous);
    return result;
}

}  // namespace

std::uint32_t EfEffectSystemTreeLifecycle::InitializeFirst(
    std::uint32_t tree, std::uint8_t first, std::uint8_t second,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeTree(tree, EfEffectSystemTreeKind::First,
                          first, second, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeEffectObject(
    std::uint32_t tree, std::uint8_t first, std::uint8_t second,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeTree(tree, EfEffectSystemTreeKind::EffectObject,
                          first, second, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeNameCount(
    std::uint32_t tree, std::uint8_t first, std::uint8_t second,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeTree(tree, EfEffectSystemTreeKind::NameCount,
                          first, second, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupArenaSequence(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::ArenaSequence, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupEffectObject(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::EffectObject, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupNameCount(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::NameCount, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupPrimaryOnUnwind(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::Primary, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupSecondaryOnUnwind(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::Secondary, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupPrimaryDirect(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::Primary, runtime, true);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupSecondaryDirect(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::Secondary, runtime, true);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupPrimaryPlain(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::Primary, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupSecondaryPlain(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::Secondary, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeAuxiliaryFirst(
    std::uint32_t tree, std::uint8_t first, std::uint8_t second,
    std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime) {
    return InitializeWrapper(tree, EfEffectSystemTreeWrapperKind::AuxiliaryFirst,
                             first, second, mode, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeInstances(
    std::uint32_t tree, std::uint8_t first, std::uint8_t second,
    std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime) {
    return InitializeWrapper(tree, EfEffectSystemTreeWrapperKind::Instances,
                             first, second, mode, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeActive(
    std::uint32_t tree, std::uint8_t first, std::uint8_t second,
    std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime) {
    return InitializeWrapper(tree, EfEffectSystemTreeWrapperKind::Active,
                             first, second, mode, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeSecondary(
    std::uint32_t tree, std::uint8_t first, std::uint8_t second,
    std::uint8_t mode, IEfEffectSystemTreeWrapperRuntime& runtime) {
    return InitializeWrapper(tree, EfEffectSystemTreeWrapperKind::Secondary,
                             first, second, mode, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseEffectObjectRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    IEfEffectSystemRangeRuntime& runtime) {
    return EraseRange(tree, result, first, last, nil,
                      EfEffectSystemRangeKind::EffectObject, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseKeyRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    IEfEffectSystemRangeRuntime& runtime) {
    return EraseRange(tree, result, first, last, nil,
                      EfEffectSystemRangeKind::Key, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::ErasePrimaryRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    IEfEffectSystemRangeRuntime& runtime) {
    return EraseRange(tree, result, first, last, nil,
                      EfEffectSystemRangeKind::Primary, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseSecondaryRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    IEfEffectSystemRangeRuntime& runtime) {
    return EraseRange(tree, result, first, last, nil,
                      EfEffectSystemRangeKind::Secondary, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseAuxiliaryFirstRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    IEfEffectSystemRangeRuntime& runtime) {
    return EraseRange(tree, result, first, last, nil,
                      EfEffectSystemRangeKind::AuxiliaryFirst, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseAuxiliarySecondRange(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t first, std::uint32_t last, std::uint32_t nil,
    IEfEffectSystemRangeRuntime& runtime) {
    return EraseRange(tree, result, first, last, nil,
                      EfEffectSystemRangeKind::AuxiliarySecond, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::AdvanceKey(
    std::uint32_t& iterator, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    const auto step = MonsterManagerTreeCursor::Successor(
        iterator, nil, [&](std::uint32_t address) {
            return runtime.ReadWord(address);
        });
    iterator = step.next;
    return step.result;
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeAuxiliaryStorage(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeStorage(tree, EfEffectSystemTreeKind::AuxiliaryStorage,
                             20u, 16u, true, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeArenaSequenceStorage(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeStorage(tree, EfEffectSystemTreeKind::ArenaSequence,
                             36u, 32u, true, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeEffectObjectStorage(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeStorage(tree, EfEffectSystemTreeKind::EffectObject,
                             36u, 32u, true, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeKeyStorage(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeStorage(tree, EfEffectSystemTreeKind::KeyStorage,
                             20u, 16u, true, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::MinimumKeyFromLink(
    std::uint32_t link, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    auto minimum = link;
    for (auto node = runtime.ReadWord(link); node != nil;
         node = runtime.ReadWord(node))
        minimum = node;
    return minimum;
}

std::uint32_t EfEffectSystemTreeLifecycle::MinimumEffectObjectFromLink(
    std::uint32_t link, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return MinimumKeyFromLink(link, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::ErasePrimaryNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime);
    return result;
}

std::uint32_t EfEffectSystemTreeLifecycle::DeletePrimarySubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializePrimaryStorage(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeStorage(tree, EfEffectSystemTreeKind::Primary,
                             36u, 32u, true, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::AdvancePrimaryAndCapture(
    std::uint32_t& iterator, std::uint32_t result, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return AdvanceAndCapture(iterator, result, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseSecondaryNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime);
    return result;
}

std::uint32_t EfEffectSystemTreeLifecycle::DeleteSecondarySubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::InitializeSecondaryStorage(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return InitializeStorage(tree, EfEffectSystemTreeKind::Secondary,
                             36u, 32u, true, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::AdvanceSecondaryAndCapture(
    std::uint32_t& iterator, std::uint32_t result, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return AdvanceAndCapture(iterator, result, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::MinimumPrimaryFromLink(
    std::uint32_t link, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return MinimumKeyFromLink(link, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::MinimumSecondaryFromLink(
    std::uint32_t link, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return MinimumKeyFromLink(link, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::AdvancePrimary(
    std::uint32_t& iterator, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return AdvanceKey(iterator, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::AdvanceSecondary(
    std::uint32_t& iterator, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return AdvanceKey(iterator, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseAuxiliaryFirstNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime, 16u, false);
    return result;
}

std::uint32_t EfEffectSystemTreeLifecycle::DeleteAuxiliaryFirstSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EfEffectSystemTreeLifecycle::EraseAuxiliarySecondNode(
    std::uint32_t tree, std::uint32_t result,
    std::uint32_t node, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    ArenaSequenceTree::EraseNode(tree, result, node, nil, runtime, 20u, false);
    return result;
}

std::uint32_t EfEffectSystemTreeLifecycle::DeleteAuxiliarySecondSubtree(
    std::uint32_t root, std::uint32_t nil,
    IArenaSequenceRuntime& runtime) {
    return ArenaSequenceTree::DeleteSubtree(root, nil, runtime, false);
}

std::uint32_t EfEffectSystemTreeLifecycle::MinimumAuxiliaryFirstFromLink(
    std::uint32_t link, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return MinimumKeyFromLink(link, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::MinimumAuxiliarySecondFromLink(
    std::uint32_t link, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return MinimumKeyFromLink(link, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::AdvanceAuxiliaryFirst(
    std::uint32_t& iterator, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return AdvanceKey(iterator, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::AdvanceAuxiliarySecond(
    std::uint32_t& iterator, std::uint32_t nil,
    IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return AdvanceKey(iterator, nil, runtime);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupAuxiliaryFirst(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::AuxiliaryFirst,
                       runtime, true);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupAuxiliarySecond(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::AuxiliarySecond,
                       runtime, true);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupArenaSequenceWithInstances(
    std::uint32_t tree, IEfEffectSystemInstanceTreeRuntime& runtime) {
    runtime.CleanupInstances(tree);
    return CleanupTree(tree, EfEffectSystemTreeKind::ArenaSequence,
                       runtime, true);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupArenaSequenceDirect(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::ArenaSequence,
                       runtime, true);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupEffectObjectDirect(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::EffectObject,
                       runtime, true);
}

std::uint32_t EfEffectSystemTreeLifecycle::CleanupNameCountDirect(
    std::uint32_t tree, IEfEffectSystemTreeLifecycleRuntime& runtime) {
    return CleanupTree(tree, EfEffectSystemTreeKind::NameCount,
                       runtime, true);
}

}  // namespace pal4
