#include "pal4/event_router_registry.h"

namespace pal4 {

std::uint32_t EventRouterRegistry::Register(
    const std::uint32_t router, const std::uint32_t mode,
    const std::uint32_t key, const std::uint32_t owner,
    const std::uint32_t callback, const std::uint32_t context,
    IEventRouterRegistryRuntime& runtime) {
    auto group = runtime.FindGroup(router, key);
    if (group == 0u)
        group = runtime.CreateGroup(router, key,
                                    static_cast<std::uint8_t>(mode));

    const auto event = runtime.Allocate(12u);
    if (event != 0u) {
        runtime.WriteWord(event + 4u, 0u);
        runtime.WriteWord(event + 8u, 0u);
        runtime.WriteWord(event, kEventVtable);
        runtime.WriteWord(event + 4u, runtime.CreateScriptEvent(
            owner, callback, context, event + 8u));
    }

    const auto head = runtime.ReadWord(group + 20u);
    const auto position = runtime.FindPosition(runtime.ReadWord(head),
                                               head, mode);
    const auto previous = runtime.ReadWord(position + 4u);
    const auto node = runtime.Allocate(20u);
    runtime.WriteWord(node, position);
    runtime.WriteWord(node + 4u, previous == 0u ? node : previous);
    runtime.WriteWord(position + 4u, node);
    runtime.WriteWord(runtime.ReadWord(node + 4u), node);
    runtime.WriteByte(node + 8u, event != 0u ? 1u : 0u);
    runtime.WriteWord(node + 12u, event);
    runtime.WriteWord(node + 16u, mode);
    runtime.WriteWord(group + 24u, runtime.ReadWord(group + 24u) + 1u);
    return event;
}

}  // namespace pal4
