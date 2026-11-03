#include "pal4/event_subscription_set.h"

namespace pal4 {

void EventSubscriptionSet::Clear(
    const std::uint32_t owner, const std::uint32_t first_offset,
    const std::uint32_t count, IEventSubscriptionRuntime& runtime) {
    if (runtime.TryClearOwnedSlots(owner, first_offset, count)) return;
    for (std::uint32_t index = 0u; index < count; ++index) {
        const auto slot = owner + first_offset + 8u * index;
        const auto handle = runtime.ReadWord(slot);
        if (handle == 0u) continue;
        const auto router = runtime.EventRouter();
        const auto group = runtime.FindEventGroup(
            router, runtime.ReadWord(slot + 4u));
        if (group != 0u) {
            const auto head = runtime.ReadWord(group + 4u);
            for (auto node = runtime.ReadWord(head); node != head;
                 node = runtime.ReadWord(node)) {
                if (runtime.ReadWord(node + 12u) != handle) continue;
                const auto previous = runtime.ReadWord(node + 4u);
                const auto next = runtime.ReadWord(node);
                runtime.WriteWord(previous, next);
                runtime.WriteWord(next + 4u, previous);
                runtime.ReleaseOwnedSlot(node + 8u);
                runtime.FreeNode(node);
                runtime.WriteWord(group + 8u,
                                  runtime.ReadWord(group + 8u) - 1u);
                break;
            }
        }
        runtime.WriteWord(slot, 0u);
        runtime.WriteWord(slot + 4u, 0u);
    }
}

}  // namespace pal4
