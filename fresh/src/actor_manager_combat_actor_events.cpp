#include "pal4/actor_manager_combat_actor_events.h"

namespace pal4 {

void ActorManagerCombatActorEvents::Summon(
    const std::uint32_t first, const std::uint32_t second,
    IActorManagerCombatSummonRuntime& runtime) {
    if (first && second)
        runtime.DispatchPair("summon", first, second, 1u);
}

std::uint32_t ActorManagerCombatActorEvents::JoinStageAndScene(
    const std::uint32_t actor, const std::uint32_t,
    IActorManagerCombatJoinRuntime& runtime) {
    const auto first = runtime.InvokeActorSlot48(actor, 0u);
    runtime.InvokeActorSlot68(actor, 7u, first);
    const auto second = runtime.InvokeActorSlot48(actor, 2u);
    runtime.InvokeActorSlot68(actor, 9u, second);
    runtime.InvokeActorSlot580(actor);
    (void)runtime.DispatchActor("me_join_stage", actor, 1u);
    return runtime.DispatchActor("me_join_scene", actor, 1u);
}

std::uint32_t ActorManagerCombatActorEvents::LeaveStageAndScene(
    const std::uint32_t actor, IActorManagerCombatJoinRuntime& runtime) {
    (void)runtime.DispatchActor("me_leave_stage", actor, 1u);
    return runtime.DispatchActor("me_leave_scene", actor, 1u);
}

}  // namespace pal4
