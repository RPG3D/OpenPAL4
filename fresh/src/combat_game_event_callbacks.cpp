#include "pal4/combat_game_event_callbacks.h"

namespace pal4 {

namespace {

std::uint32_t CountTargetAction(
    std::uint32_t object, std::uint32_t event, std::uint32_t stat,
    ICombatGameEventCallbackRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    if (runtime.ReadWord(actor + 804u) == 1u)
        (void)runtime.UpdateStat(object, stat, 1u);
    const auto target = runtime.ReadWord(event + 4u);
    if (runtime.ReadWord(target + 804u) != 1u) return target;
    const auto index = runtime.ReadWord(target + 1192u);
    const auto slot = object + 0x248u + index * 4u;
    runtime.WriteWord(slot, runtime.ReadWord(slot) + 1u);
    return slot;
}

std::uint32_t UpdateForActor(
    std::uint32_t object, std::uint32_t event,
    std::uint32_t state, std::uint32_t stat,
    ICombatGameEventCallbackRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    if (runtime.ReadWord(actor + 804u) == state)
        return runtime.UpdateStat(object, stat, 1u);
    return actor;
}

}  // namespace

std::uint32_t CombatGameEventCallbacks::OnEvent21(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return CountTargetAction(object, event, 21u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnEvent7(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return CountTargetAction(object, event, 7u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnEvent6(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return CountTargetAction(object, event, 6u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnEvent8(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return CountTargetAction(object, event, 8u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnFlee(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    const auto stat = runtime.ReadWord(actor + 804u) == 2u ? 18u : 11u;
    (void)runtime.UpdateStat(object, stat, 1u);
    const auto target = runtime.ReadWord(event + 4u);
    if (runtime.ReadWord(target + 804u) != 1u) return target;
    const auto index = runtime.ReadWord(target + 1192u);
    if (index < 4u) {
        const auto slot = object + 0x228u + index * 4u;
        runtime.WriteWord(slot, runtime.ReadWord(slot) + 1u);
    }
    return index;
}

std::uint32_t CombatGameEventCallbacks::OnGuard(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return CountTargetAction(object, event, 2u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnDeath(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    const auto target = runtime.ReadWord(event + 4u);
    if (runtime.ReadWord(target + 804u) == 1u)
        return runtime.UpdateStat(object, 4u, 1u);
    return event;
}

std::uint32_t CombatGameEventCallbacks::OnRoundStart(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return UpdateForActor(object, event, 1u, 1u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnBattleStart(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return UpdateForActor(object, event, 1u, 0u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnSpecial(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return UpdateForActor(object, event, 1u, 10u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnEscape(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return UpdateForActor(object, event, 2u, 17u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnPlayerDeath(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    const auto index = runtime.ReadWord(actor + 1192u);
    if (index < 4u) {
        const auto slot = object + 0x238u + index * 4u;
        runtime.WriteWord(slot, runtime.ReadWord(slot) + 1u);
    }
    return index;
}

std::uint32_t CombatGameEventCallbacks::OnDodge(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    const auto target = runtime.ReadWord(event + 4u);
    if (runtime.ReadWord(target + 804u) == 1u)
        return runtime.UpdateStat(object, 3u, 1u);
    return event;
}

std::uint32_t CombatGameEventCallbacks::OnAttack(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return UpdateForActor(object, event, 1u, 5u, runtime);
}

std::uint32_t CombatGameEventCallbacks::OnItemUse(
    std::uint32_t object, std::uint32_t event,
    ICombatGameEventCallbackRuntime& runtime) {
    return UpdateForActor(object, event, 1u, 9u, runtime);
}

}  // namespace pal4
