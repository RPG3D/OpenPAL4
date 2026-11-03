#include "pal4/actor_manager_player_stat_refresh.h"

namespace pal4 {

std::int32_t ActorManagerPlayerStatRefresh::Rebuild(
    const std::uint32_t actor, IActorManagerPlayerStatRuntime& runtime) {
    const auto storage = runtime.Allocate(12u);
    std::uint32_t damage = 0u;
    try {
        if (storage) damage = runtime.ConstructDamage(storage);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }
    const auto temporary = runtime.TemporarySlotAddress();
    runtime.WriteByte(temporary, damage ? 1u : 0u);
    runtime.WriteWord(temporary + 4u, damage);
    try {
        (void)ActorManagerOwnedSlot::MoveAssign(actor + 796u, temporary,
                                                 runtime);
    } catch (...) {
        (void)ActorManagerOwnedSlot::Destroy(temporary, 0u, runtime);
        throw;
    }
    (void)ActorManagerOwnedSlot::Destroy(temporary, 0u, runtime);

    const auto value7 = runtime.ActorSlot52(actor, 7u);
    const auto value9 = runtime.ActorSlot52(actor, 9u);
    const auto value8 = runtime.ActorSlot52(actor, 8u);
    const auto data = actor + 340u;
    runtime.CopyBytes(data, actor + 4u, 0x150u);
    runtime.ProcessInventoryItemSlots(actor + 896u, data, actor + 4u);
    runtime.ProcessSkills(actor, 1u, actor);

    runtime.WriteWord(actor + 572u, static_cast<std::uint32_t>(value7));
    const auto max7 = runtime.DataElement(data, 0u);
    if (value7 > max7)
        runtime.WriteWord(actor + 572u, static_cast<std::uint32_t>(
                              runtime.DataElement(data, 0u)));
    else if (value7 < 0)
        runtime.WriteWord(actor + 572u, 0u);

    runtime.WriteWord(actor + 580u, static_cast<std::uint32_t>(value9));
    const auto max9 = runtime.DataElement(data, 2u);
    if (value9 > max9)
        runtime.WriteWord(actor + 580u, static_cast<std::uint32_t>(
                              runtime.DataElement(data, 2u)));
    else if (value9 < 0)
        runtime.WriteWord(actor + 580u, 0u);

    runtime.SetValidatedProperty(data, 8u,
                                 static_cast<std::uint32_t>(value8));
    const auto current7 = static_cast<std::int32_t>(
        runtime.ReadWord(actor + 572u));
    if (current7 > runtime.DataElement(data, 0u))
        runtime.SetValidatedProperty(data, 7u,
                                     static_cast<std::uint32_t>(
                                         runtime.DataElement(data, 0u)));

    const auto current_max9 = runtime.DataElement(data, 2u);
    if (runtime.GetProperty(data, 9u) > current_max9)
        runtime.SetValidatedProperty(data, 9u,
                                     static_cast<std::uint32_t>(
                                         runtime.DataElement(data, 0u)));
    return runtime.UpdateHealthStatus(actor);
}

}  // namespace pal4
