#include "pal4/actor_manager_combat_animation_lookup.h"

#include <string>

namespace pal4 {

std::uint32_t ActorManagerCombatAnimationLookup::Find(
    const std::uint32_t actor, const std::uint32_t source,
    const std::uint32_t text_offset,
    IActorManagerCombatAnimationLookupRuntime& runtime) {
    const auto record = source + text_offset;
    std::string name(runtime.ReadText(record));
    name += ".anm";
    const auto child = runtime.ReadWord(actor + 808u);
    return runtime.FindAnimation(child, name) ? record : kMissingAnimation;
}

std::uint32_t ActorManagerCombatAnimationLookup::FromPrimaryName(
    const std::uint32_t actor, const std::uint32_t source,
    IActorManagerCombatAnimationLookupRuntime& runtime) {
    return Find(actor, source, 88u, runtime);
}

std::uint32_t ActorManagerCombatAnimationLookup::FromSecondaryName(
    const std::uint32_t actor, const std::uint32_t source,
    IActorManagerCombatAnimationLookupRuntime& runtime) {
    return Find(actor, source, 144u, runtime);
}

}  // namespace pal4
