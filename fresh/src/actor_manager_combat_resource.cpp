#include "pal4/actor_manager_combat_actor_events.h"

namespace pal4 {
std::uint32_t ActorManagerCombatActorEvents::RefreshResource(
    const std::uint32_t actor, IActorManagerCombatResourceRuntime& runtime) {
    const auto stored_name = runtime.ReadWord(actor + 1004u);
    runtime.InvokeActorSlot592(actor,
        stored_name ? stored_name : kEmptyText);
    const auto child = runtime.ReadWord(actor + 808u);
    if (!child) return 0u;
    const auto value = runtime.InvokeActorSlot444(actor, 0u);
    return runtime.InvokeChildSlot4(child, value);
}

} // namespace pal4
