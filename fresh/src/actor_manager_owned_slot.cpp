#include "pal4/actor_manager_owned_slot.h"

namespace pal4 {

void ActorManagerOwnedSlot::Release(
    const std::uint32_t slot, IActorManagerOwnedSlotRuntime& runtime) {
    static_cast<void>(Destroy(slot, 0u, runtime));
}

std::uint32_t ActorManagerOwnedSlot::MoveConstructNullable(
    std::uint32_t destination, std::uint32_t source,
    IActorManagerOwnedSlotRuntime& runtime) {
    return destination == 0u ? 0u : MoveConstruct(destination, source, runtime);
}

std::uint32_t ActorManagerOwnedSlot::MoveConstruct(
    std::uint32_t destination, std::uint32_t source,
    IActorManagerOwnedSlotRuntime& runtime) {
    const auto owner = runtime.ReadByte(source);
    runtime.WriteByte(destination, owner);
    runtime.WriteByte(source, 0u);
    runtime.WriteWord(destination + 4u, runtime.ReadWord(source + 4u));
    return destination;
}

std::uint32_t ActorManagerOwnedSlot::Destroy(
    std::uint32_t slot, std::uint8_t flags,
    IActorManagerOwnedSlotRuntime& runtime) {
    if (runtime.ReadByte(slot) != 0u) {
        const auto actor = runtime.ReadWord(slot + 4u);
        if (actor != 0u) runtime.DeleteActor(actor, 1u);
    }
    if ((flags & 1u) != 0u) runtime.Free(slot);
    return slot;
}

std::uint32_t ActorManagerOwnedSlot::DestroyEventPayload(
    const std::uint32_t slot, const std::uint8_t flags,
    IActorManagerOwnedSlotRuntime& runtime) {
    return Destroy(slot, flags, runtime);
}

std::uint32_t ActorManagerOwnedSlot::MoveAssign(
    std::uint32_t destination, std::uint32_t source,
    IActorManagerOwnedSlotRuntime& runtime) {
    if (destination == source) return destination;
    const auto old_actor = runtime.ReadWord(destination + 4u);
    const auto new_actor = runtime.ReadWord(source + 4u);
    const auto source_owner = runtime.ReadByte(source);
    if (old_actor == new_actor) {
        if (source_owner != 0u) runtime.WriteByte(destination, 1u);
    } else {
        if (runtime.ReadByte(destination) != 0u && old_actor != 0u)
            runtime.DeleteActor(old_actor, 1u);
        runtime.WriteByte(destination, source_owner);
    }
    runtime.WriteByte(source, 0u);
    runtime.WriteWord(destination + 4u, new_actor);
    return destination;
}

}  // namespace pal4
