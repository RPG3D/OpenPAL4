#include "pal4/actor_manager_combat_actor.h"

namespace pal4 {

std::int32_t ActorManagerCombatActor::RebuildStats(
    const std::uint32_t object,
    IActorManagerCombatStatsRuntime& runtime) {
    const auto storage = runtime.Allocate(12u);
    std::uint32_t damage = 0u;
    try {
        if (storage != 0u) damage = runtime.ConstructDamage(storage);
    } catch (...) {
        runtime.Free(storage);
        throw;
    }

    const auto temporary = runtime.TemporarySlotAddress();
    runtime.WriteByte(temporary, damage ? 1u : 0u);
    runtime.WriteWord(temporary + 4u, damage);
    try {
        (void)ActorManagerOwnedSlot::MoveAssign(object + 796u, temporary,
                                                 runtime);
    } catch (...) {
        (void)ActorManagerOwnedSlot::Destroy(temporary, 0u, runtime);
        throw;
    }
    (void)ActorManagerOwnedSlot::Destroy(temporary, 0u, runtime);

    const auto old_value9 = runtime.ReadWord(object + 580u);
    const auto old_value8 = runtime.ReadWord(object + 576u);
    const auto old_health = runtime.ReadWord(object + 572u);
    const auto data = object + 340u;
    runtime.CopyBytes(data, object + 4u, 336u);
    runtime.ProcessSkills(object, 1u);

    runtime.WriteWord(object + 572u, old_health);
    const auto maximum_health = runtime.DataElement(data, 0u);
    if (static_cast<std::int32_t>(old_health) > maximum_health)
        runtime.WriteWord(object + 572u,
                          static_cast<std::uint32_t>(
                              runtime.DataElement(data, 0u)));
    else if (static_cast<std::int32_t>(old_health) < 0)
        runtime.WriteWord(object + 572u, 0u);

    runtime.SetValidatedProperty(data, 9u, old_value9);
    runtime.SetValidatedProperty(data, 8u, old_value8);
    const auto current_health = runtime.ReadWord(object + 572u);
    const auto current_maximum_health = runtime.DataElement(data, 0u);
    if (static_cast<std::int32_t>(current_health) > current_maximum_health) {
        runtime.SetValidatedProperty(data, 7u,
            static_cast<std::uint32_t>(runtime.DataElement(data, 0u)));
    }
    const auto current_value9 = runtime.ReadWord(object + 580u);
    const auto current_maximum_value9 = runtime.DataElement(data, 2u);
    if (static_cast<std::int32_t>(current_value9) > current_maximum_value9) {
        runtime.SetValidatedProperty(data, 9u,
            runtime.ReadWord(object + 344u));
    }
    return runtime.UpdateHealthStatus(object);
}

}  // namespace pal4
