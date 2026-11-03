#include "pal4/actor_manager_property_events.h"

#include <array>

namespace pal4 {
namespace {

std::uint32_t EmitImmunity(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    std::string_view event, IActorManagerPropertyEventRuntime& runtime) {
    const std::array payload{actor + 716u, second, first, third};
    return runtime.Dispatch(event, payload, 1u);
}

std::uint32_t Forward(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third, std::uint32_t slot,
    IActorManagerPropertyEventRuntime& runtime) {
    return runtime.InvokeProperty(runtime.ReadWord(actor + 800u),
                                  slot, actor, first, second, third);
}

}  // namespace

std::uint32_t ActorManagerPropertyEvents::AddSkill(
    std::uint32_t actor, std::uint32_t context,
    std::uint32_t extra, std::uint32_t skill,
    IActorManagerPropertyEventRuntime& runtime) {
    runtime.WriteByte(actor + 892u, 1u);
    runtime.AddSkillNative(actor, context, skill);
    const std::array payload{actor + 716u, skill, extra};
    return runtime.Dispatch("ae_add_skill", payload, 1u);
}

std::uint32_t ActorManagerPropertyEvents::RemoveSkill(
    std::uint32_t actor, std::uint32_t context, std::uint32_t skill,
    IActorManagerPropertyEventRuntime& runtime) {
    runtime.WriteByte(actor + 892u, 1u);
    runtime.RemoveSkillNative(actor, context, skill);
    const std::array payload{actor + 716u, skill};
    return runtime.Dispatch("ae_remove_skill", payload, 1u);
}

std::uint32_t ActorManagerPropertyEvents::ImmunityStateA(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerPropertyEventRuntime& runtime) {
    return EmitImmunity(actor, first, second, third,
                        "ae_immunity_state_a", runtime);
}

std::uint32_t ActorManagerPropertyEvents::ImmunityStateB(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerPropertyEventRuntime& runtime) {
    return EmitImmunity(actor, first, second, third,
                        "ae_immunity_state_b", runtime);
}

std::uint32_t ActorManagerPropertyEvents::ImmunitySkill(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerPropertyEventRuntime& runtime) {
    return EmitImmunity(actor, first, second, third,
                        "ae_immunity_skill", runtime);
}

std::uint32_t ActorManagerPropertyEvents::ImmunityNormal(
    std::uint32_t actor, IActorManagerPropertyEventDispatchRuntime& runtime) {
    const std::array payload{actor + 716u};
    return runtime.Dispatch("ae_immunity_normal", payload, 1u);
}

std::uint32_t ActorManagerPropertyEvents::Wounded(
    std::uint32_t actor, std::uint32_t argument,
    IActorManagerPropertyEventRuntime& runtime) {
    return runtime.RegisterActorEvent(
        runtime.CombatWounded(actor, argument));
}

std::uint32_t ActorManagerPropertyEvents::ForwardSlot96(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerPropertyEventRuntime& runtime) {
    return Forward(actor, first, second, third, 96u, runtime);
}

std::uint32_t ActorManagerPropertyEvents::ForwardSlot100(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerPropertyEventRuntime& runtime) {
    return Forward(actor, first, second, third, 100u, runtime);
}

std::uint32_t ActorManagerPropertyEvents::ForwardSlot104(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerPropertyEventRuntime& runtime) {
    return Forward(actor, first, second, third, 104u, runtime);
}

std::uint32_t ActorManagerPropertyEvents::ForwardSlot108(
    std::uint32_t actor, std::uint32_t first,
    std::uint32_t second, std::uint32_t third,
    IActorManagerPropertyEventRuntime& runtime) {
    return Forward(actor, first, second, third, 108u, runtime);
}

}  // namespace pal4
