#include "pal4/actor_manager_attack_events.h"

#include <array>

namespace pal4 {
namespace {

void RouteOne(std::uint32_t actor, std::uint32_t target,
              std::string_view event, std::uint32_t slot,
              IActorManagerAttackEventRuntime& runtime) {
    if (target == 0u) return;
    const std::array payload{actor + 716u, target};
    runtime.Dispatch(event, payload, 1u);
    runtime.InvokeProperty(runtime.ReadWord(actor + 800u),
                           slot, actor, target, 0u);
}

void RouteTwo(std::uint32_t actor, std::uint32_t target,
              std::uint32_t detail, std::string_view event,
              std::uint32_t slot,
              IActorManagerAttackEventRuntime& runtime) {
    if (detail == 0u || target == 0u) return;
    // Original pair event is {facet,target,record}; property call is {actor,record,target}.
    const std::array payload{actor + 716u, target, detail};
    runtime.Dispatch(event, payload, 1u);
    runtime.InvokeProperty(runtime.ReadWord(actor + 800u),
                           slot, actor, detail, target);
}

}  // namespace

void ActorManagerAttackEvents::PhysicalAttack(
    std::uint32_t actor, std::uint32_t target,
    IActorManagerAttackEventRuntime& runtime) {
    RouteOne(actor, target, "ae_physical_attack", 52u, runtime);
}

void ActorManagerAttackEvents::CriticalAttack(
    std::uint32_t actor, std::uint32_t target,
    IActorManagerAttackEventRuntime& runtime) {
    RouteOne(actor, target, "ae_critical_attack", 56u, runtime);
}

void ActorManagerAttackEvents::FendOffed(
    std::uint32_t actor, std::uint32_t target,
    IActorManagerAttackEventRuntime& runtime) {
    RouteOne(actor, target, "ae_fend_offed", 60u, runtime);
}

void ActorManagerAttackEvents::AttackMissed(
    std::uint32_t actor, std::uint32_t target,
    IActorManagerAttackEventRuntime& runtime) {
    RouteOne(actor, target, "ae_attack_missed", 64u, runtime);
}

void ActorManagerAttackEvents::CastMagic(
    std::uint32_t actor, std::uint32_t target, std::uint32_t magic,
    IActorManagerAttackEventRuntime& runtime) {
    RouteTwo(actor, target, magic, "ae_cast_magic", 68u, runtime);
}

void ActorManagerAttackEvents::Stunt(
    std::uint32_t actor, std::uint32_t target, std::uint32_t stunt,
    IActorManagerAttackEventRuntime& runtime) {
    RouteTwo(actor, target, stunt, "ae_stunt", 72u, runtime);
}

void ActorManagerAttackEvents::UseProperty(
    std::uint32_t actor, std::uint32_t target, std::uint32_t property,
    IActorManagerAttackEventRuntime& runtime) {
    RouteTwo(actor, target, property, "ae_use_property", 76u, runtime);
}

}  // namespace pal4
