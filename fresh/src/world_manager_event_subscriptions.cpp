#include "pal4/world_manager_event_subscriptions.h"

#include <array>

namespace pal4 {

std::uint32_t WorldManagerEventSubscriptions::Subscribe(
    const std::uint32_t manager, IWorldManagerEventRuntime& runtime) {
    (void)Unsubscribe(manager, runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto& binding = kBindings[index];
        const auto key = runtime.EventKey(binding.name);
        result = runtime.Subscribe(runtime.EventRouter(), 3u, key,
                                   manager, binding.callback);
        runtime.WriteWord(manager + 84u + 8u * index, result);
        runtime.WriteWord(manager + 88u + 8u * index, key);
    }
    return result;
}

std::uint32_t WorldManagerEventSubscriptions::Unsubscribe(
    const std::uint32_t manager, IWorldManagerEventRuntime& runtime) {
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto slot = manager + 84u + 8u * index;
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
    return 0u;
}

}  // namespace pal4
