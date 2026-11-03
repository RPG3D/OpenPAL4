#include "pal4/actor_manager_facet_action_events.h"

namespace pal4 {

std::uint8_t ActorManagerFacetActionEvents::CanTakeAction(
    const std::uint32_t facet,
    IActorManagerFacetActionEventRuntime& runtime) {
    const auto actor = facet - kActorOffset;
    runtime.WriteWord(facet + kPendingOffset, 0u);
    runtime.HandleStateEvent(facet);
    const auto state = runtime.CheckState3(actor);
    if (state) return state;
    if (runtime.ReadWord(facet + kPendingOffset)) {
        runtime.ProcessActorEvents(actor);
        runtime.WriteWord(facet + kPendingOffset, 0u);
    }
    return static_cast<std::uint8_t>(
        runtime.Dispatch("me_can_take_action", actor, 1u));
}

std::uint32_t ActorManagerFacetActionEvents::MovedAtActionSequence(
    const std::uint32_t facet, const std::int32_t value,
    IActorManagerFacetProgressRuntime& runtime) {
    const auto actor = facet - kActorOffset;
    const auto state = runtime.ActorStateSlot20(actor);
    if (state < 0 || state >= 6) return static_cast<std::uint32_t>(state);
    runtime.SetProperty2(facet, value);
    return runtime.Dispatch("me_moved_at_action_sequence", actor, 3u);
}

std::uint8_t ActorManagerPlayerFacetActionEvents::CanTakeAction(
    const std::uint32_t facet,
    IActorManagerPlayerFacetActionEventRuntime& runtime) {
    const auto actor = facet - kActorOffset;
    if (static_cast<std::int32_t>(
            runtime.ReadWord(facet + kCombatThresholdOffset)) >= 50 &&
        runtime.CombatSystemInactive())
        return static_cast<std::uint8_t>(runtime.ActorSlot532(actor));
    runtime.HandleStateEvent(facet);
    const auto state = runtime.CheckState3(actor);
    if (state) return state;
    return static_cast<std::uint8_t>(
        runtime.Dispatch("re_can_take_action", actor, 1u));
}

std::uint32_t ActorManagerPlayerFacetActionEvents::MovedAtActionSequence(
    const std::uint32_t facet, const std::int32_t value,
    IActorManagerFacetProgressRuntime& runtime) {
    const auto actor = facet - kActorOffset;
    const auto state = runtime.ActorStateSlot20(actor);
    if (state < 0 || state >= 3) return static_cast<std::uint32_t>(state);
    runtime.SetProperty2(facet, value);
    return runtime.Dispatch("re_moved_at_action_sequence", actor, 3u);
}

}  // namespace pal4
