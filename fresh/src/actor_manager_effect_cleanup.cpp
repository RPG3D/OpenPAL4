#include "pal4/actor_manager_effect_cleanup.h"

#include <array>

namespace pal4 {

std::uint32_t ActorManagerEffectCleanup::Release(
    std::uint32_t actor, IActorManagerEffectCleanupRuntime& runtime) {
    const auto tree = actor + 836u;
    const auto head = runtime.ReadWord(tree + 4u);
    for (auto cursor = runtime.ReadWord(head); cursor != head;
         cursor = runtime.AdvanceTreeCursor(cursor)) {
        runtime.ResetDynamicEffect(cursor + 16u);
    }

    if (runtime.ReadWord(tree + 12u) != 0u) {
        runtime.ClearTreeRange(tree, runtime.ReadWord(head + 4u));
        runtime.WriteWord(head + 4u, runtime.NilNode());
        runtime.WriteWord(tree + 12u, 0u);
        runtime.WriteWord(head, head);
        runtime.WriteWord(head + 8u, head);
    } else {
        auto cursor = runtime.ReadWord(head);
        while (cursor != head) {
            const auto node = cursor;
            cursor = runtime.AdvanceTreeCursor(cursor);
            runtime.EraseTreeNode(tree, node);
        }
    }

    for (const auto offset : std::array{868u, 876u, 884u})
        runtime.UpdateEffect(actor + offset);
    runtime.WriteWord(actor + 832u, 0u);
    (void)runtime.InvokeActorSlot(actor, 576u, 0u);
    return runtime.InvokeActorSlot(actor, 24u, 6u);
}

}  // namespace pal4
