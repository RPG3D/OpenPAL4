#include "pal4/actor_manager_player_property_access.h"

namespace pal4 {

std::uint32_t ActorManagerPlayerPropertyAccess::OwnedChild(
    const std::uint32_t actor,
    IActorManagerPlayerPropertyAccessRuntime& runtime) {
    return runtime.ReadWord(actor + 1316u);
}

std::uint32_t ActorManagerPlayerPropertyAccess::ForwardProperty(
    const std::uint32_t actor, const std::uint32_t property,
    const std::uint32_t value,
    IActorManagerPlayerPropertyAccessRuntime& runtime) {
    if (property != 4u)
        return runtime.ForwardPropertySlot168(actor, property, value);
    runtime.ActorSlot52(actor, 4u);
    return runtime.ObjectSlot168(runtime.ReadWord(actor + 800u), actor, 4u,
                                 value);
}

std::uint32_t ActorManagerPlayerPropertyAccess::IndexedValue(
    const std::uint32_t actor, const std::uint32_t index,
    IActorManagerPlayerPropertyAccessRuntime& runtime) {
    if (index > 9u) return index;
    return runtime.ReadWord(actor + 204u + index * 4u);
}

}  // namespace pal4
