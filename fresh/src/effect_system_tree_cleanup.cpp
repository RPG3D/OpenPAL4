#include "pal4/effect_system_tree_cleanup.h"

namespace pal4 {
namespace {

std::uint32_t ClearTree(EffectRegistrationTree kind,
                        std::uint32_t tree,
                        IEffectSystemTreeCleanupRuntime& runtime) {
    const auto head = runtime.ReadWord(tree + 4u);
    auto cursor = runtime.ReadWord(head);
    if (runtime.ReadWord(tree + 12u) != 0u) {
        const auto nil = runtime.Nil(kind);
        cursor = runtime.ReadWord(head + 4u);
        while (cursor != nil) {
            runtime.DeleteSubtree(kind, runtime.ReadWord(cursor + 8u));
            const auto next = runtime.ReadWord(cursor);
            runtime.Free(cursor);
            cursor = next;
        }
        runtime.WriteWord(head + 4u, nil);
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
        return runtime.FirstElement(tree);
    }
    while (cursor != head) {
        const auto next = runtime.Advance(kind, cursor);
        runtime.Erase(kind, tree, cursor);
        cursor = next;
    }
    return cursor;
}

}  // namespace

std::uint8_t EffectSystemTreeCleanup::CleanupEffects(
    std::uint32_t auxiliary, IEffectSystemTreeCleanupRuntime& runtime) {
    const auto active = runtime.ReadByte(auxiliary + 4u);
    if (active == 0u)
        return active;
    runtime.WriteByte(auxiliary + 4u, 0u);
    runtime.WriteWord(auxiliary + 8u, 0u);
    (void)ClearTree(EffectRegistrationTree::Clumps,
                    auxiliary + 12u, runtime);
    return static_cast<std::uint8_t>(
        ClearTree(EffectRegistrationTree::Atomics,
                  auxiliary + 28u, runtime));
}

}  // namespace pal4
