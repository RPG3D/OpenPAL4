#include "pal4/actor_manager_player_child_ownership.h"

namespace pal4 {

std::uint32_t ActorManagerPlayerChildOwnership::Assign(
    const std::uint32_t actor, const std::uint32_t child,
    IActorManagerOwnedSlotRuntime& runtime) {
    const auto slot = actor + 1312u;
    const auto old_child = runtime.ReadWord(slot + 4u);
    if (old_child == child) {
        if (child != 0u) runtime.WriteByte(slot, 1u);
    } else {
        if (runtime.ReadByte(slot) != 0u && old_child != 0u)
            runtime.DeleteActor(old_child, 1u);
        runtime.WriteByte(slot, child != 0u ? 1u : 0u);
    }
    runtime.WriteWord(slot + 4u, child);
    return slot;
}

}  // namespace pal4
