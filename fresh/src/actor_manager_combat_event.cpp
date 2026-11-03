#include "pal4/actor_manager_combat_event.h"

namespace pal4 {

std::uint32_t ActorManagerCombatEvent::Handle(
    const std::uint32_t facet, IActorManagerTurnStateRuntime& runtime,
    IActorAnimationControlRuntime& animation_runtime) {
    const auto actor = facet - 716u;
    runtime.WriteWord(facet + 116u,
                      runtime.ReadWord(facet + 116u) + 1u);
    runtime.WriteWord(facet - 4u, 0u);
    runtime.WriteWord(facet - 8u, 0u);
    runtime.SetStateZero(actor);
    runtime.CountScriptProperties(runtime.ReadWord(facet + 84u), actor);
    runtime.InvokeActorSlot260(actor, 6u, actor);
    runtime.ProcessDynamicList(actor, 1u);
    if (runtime.ObjectCheckState3(actor)) {
        runtime.HandleEvent14(actor, 3u, 1u);
        runtime.RegisterActorEvent(runtime.ScriptCombatDead(actor));
    }
    const auto result = runtime.InvokeActorSlot152(actor);
    const auto child = runtime.ReadWord(facet + 92u);
    if (!child) return result;
    const auto queue = runtime.CombatActorDataPtr();
    return ActorAnimationControl::SetAlpha(
        child, result == 4u ? 0.05f : 1.0f,
        queue, animation_runtime);
}

std::uint32_t ActorManagerCombatEvent::ChildSlot56(
    const std::uint32_t actor, const std::uint32_t argument,
    IActorManagerTurnStateRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? runtime.InvokeChildSlot(child, 56u, argument) : 0u;
}

std::uint32_t ActorManagerCombatEvent::ChildSlot52(
    const std::uint32_t actor, const std::uint32_t argument,
    IActorManagerTurnStateRuntime& runtime) {
    const auto child = runtime.ReadWord(actor + 808u);
    return child ? runtime.InvokeChildSlot(child, 52u, argument) : 0u;
}

}  // namespace pal4
