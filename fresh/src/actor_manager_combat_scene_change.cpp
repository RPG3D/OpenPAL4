#include "pal4/actor_manager_combat_scene_change.h"

#include <array>

namespace pal4 {

void ActorManagerCombatSceneChange::ApplyPendingRecord(
    const std::uint32_t actor,
    IActorManagerCombatSceneChangeRuntime& runtime) {
    const auto state = runtime.ActorSlot20(actor);
    const auto property7 = runtime.ActorSlot52(actor, 7u);
    const auto property9 = runtime.ActorSlot52(actor, 9u);
    const auto property8 = runtime.ActorSlot52(actor, 8u);
    const auto record = runtime.FindDatabaseRecord(
        runtime.ReadWord(actor + 1164u));
    if (!record) return;

    runtime.DispatchEvent("me_leave_scene", actor, 1u);
    runtime.ActorSlotNoArgument(actor, 588u);
    runtime.CopyFrom(actor, record);
    runtime.ActorSlotNoArgument(actor, 584u);
    runtime.ActorSlotNoArgument(actor, 276u);
    runtime.ActorSlot24(actor, state);
    runtime.ActorSlot68(actor, 7u, property7);
    runtime.ActorSlot68(actor, 9u, property9);
    runtime.ActorSlot68(actor, 8u, property8);
    runtime.ActorSlotNoArgument(actor, 580u);
    runtime.DispatchEvent("me_join_scene", actor, 1u);
    runtime.WriteWord(actor + 1164u, 0u);

    for (const auto offset : std::array{868u, 876u, 884u})
        runtime.ResetEffect(actor, actor + offset);
    const auto child = runtime.ReadWord(actor + 808u);
    if (child)
        ActorManagerEffectSetup::LoadAttachments(actor, child, runtime);
}

}  // namespace pal4
