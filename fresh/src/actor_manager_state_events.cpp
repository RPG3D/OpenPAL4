#include "pal4/actor_manager_state_events.h"

#include <array>

namespace pal4 {
namespace {

std::uint32_t EmitActor(std::uint32_t actor, std::string_view event,
                        IActorManagerStateEventRuntime& runtime) {
    const std::array payload{actor + 716u};
    return runtime.Dispatch(event, payload, 1u);
}

std::uint32_t Property(std::uint32_t actor, std::uint32_t slot,
                       IActorManagerStateEventRuntime& runtime) {
    return runtime.InvokeProperty(runtime.ReadWord(actor + 800u),
                                  slot, actor);
}

}  // namespace

std::uint8_t ActorManagerStateEvents::SetState(
    std::uint32_t actor, std::uint32_t state, bool notify,
    IActorManagerStateEventRuntime& runtime) {
    if (state <= 7u) runtime.TouchSkillSystem(actor, state);
    runtime.WriteWord(actor + 828u, state);
    if (!notify) return 0u;
    return static_cast<std::uint8_t>(
        EmitActor(actor, "ae_state_changed", runtime));
}

std::uint32_t ActorManagerStateEvents::BeginFlee(
    std::uint32_t actor, IActorManagerStateEventRuntime& runtime) {
    (void)SetState(actor, 5u, true, runtime);
    return EmitActor(actor, "ae_fleeing", runtime);
}

std::uint32_t ActorManagerStateEvents::Fled(
    std::uint32_t actor, IActorManagerStateEventRuntime& runtime) {
    (void)SetState(actor, 6u, true, runtime);
    (void)EmitActor(actor, "ae_fleed", runtime);
    return Property(actor, 80u, runtime);
}

std::uint32_t ActorManagerStateEvents::FleeFailed(
    std::uint32_t actor, IActorManagerStateEventRuntime& runtime) {
    (void)SetState(actor, 2u, true, runtime);
    return EmitActor(actor, "ae_flee_fail", runtime);
}

std::uint32_t ActorManagerStateEvents::Defend(
    std::uint32_t actor, IActorManagerStateEventRuntime& runtime) {
    (void)EmitActor(actor, "ae_defende", runtime);
    return Property(actor, 84u, runtime);
}

std::uint32_t ActorManagerStateEvents::BecomeDead(
    std::uint32_t actor, std::uint32_t cause,
    IActorManagerStateEventRuntime& runtime) {
    runtime.ResetObjectProperties(actor);
    runtime.InvokeActorSlot(actor, 584u);
    runtime.InvokeActorSlot(actor, 276u);
    (void)SetState(actor, 4u, true, runtime);
    const std::array payload{actor + 716u, cause};
    (void)runtime.Dispatch("ae_become_dead", payload, 1u);
    return Property(actor, 88u, runtime);
}

std::uint32_t ActorManagerStateEvents::BecomeAlive(
    std::uint32_t actor, IActorManagerStateEventRuntime& runtime) {
    (void)SetState(actor, 1u, true, runtime);
    (void)EmitActor(actor, "ae_become_alive", runtime);
    return Property(actor, 92u, runtime);
}

std::uint32_t ActorManagerStateEvents::ProtectAction(
    std::uint32_t actor, std::uint32_t protected_actor,
    std::uint32_t other, IActorManagerPairEventRuntime& runtime) {
    const std::array payload{protected_actor == 0u ? 0u :
                                 protected_actor + 716u,
                             other, actor};
    return runtime.Dispatch("ae_protect_action", payload, 1u);
}

std::uint32_t ActorManagerStateEvents::JointAttack(
    std::uint32_t actor, std::uint32_t other,
    IActorManagerPairEventRuntime& runtime) {
    const std::array payload{actor + 716u, other};
    return runtime.Dispatch("ae_joint_attack", payload, 1u);
}

std::uint32_t ActorManagerStateEvents::ReactAttack(
    std::uint32_t actor, std::uint32_t other,
    IActorManagerPairEventRuntime& runtime) {
    const std::array payload{actor + 716u, other};
    return runtime.Dispatch("ae_react_attack", payload, 1u);
}

}  // namespace pal4
