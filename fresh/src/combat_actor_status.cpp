#include "pal4/combat_actor_status.h"

#include "pal4/combat_health_state.h"

#include <bit>

namespace pal4 {

bool CombatActorStatus::IsStateThree(
    std::uint32_t actor, ICombatActorStatusRuntime& runtime) {
    return runtime.Query(actor, 0xa8u) == 3;
}

bool CombatActorStatus::IsNotStateThree(
    std::uint32_t actor, ICombatActorStatusRuntime& runtime) {
    return runtime.Query(actor, 0xa8u) != 3;
}

std::int32_t CombatActorStatus::SetSecondaryStateOne(
    std::uint32_t actor, ICombatActorStatusRuntime& runtime) {
    return runtime.Set(actor, 0xc4u, 1);
}

std::int32_t CombatActorStatus::SetSecondaryStateZero(
    std::uint32_t actor, ICombatActorStatusRuntime& runtime) {
    return runtime.Set(actor, 0xc4u, 0);
}

bool CombatActorStatus::IsStatusOne(
    std::uint32_t actor, ICombatActorStatusRuntime& runtime) {
    return runtime.Query(actor, 0xb0u) == 1;
}

std::int32_t CombatActorStatus::ResetStatus(
    std::uint32_t actor, ICombatActorStatusRuntime& runtime) {
    return runtime.Set(actor, 0xb4u, 0);
}

std::int32_t CombatActorStatus::QueryConditionalStatus(
    std::uint32_t actor, std::int32_t mode,
    ICombatActorStatusRuntime& runtime) {
    const auto current = runtime.QueryIndexed(actor, 0x34u, 4u);
    return std::bit_cast<std::int32_t>(
        std::bit_cast<std::uint32_t>(current) + (mode == 2 ? 9u : 4u));
}

std::int32_t CombatActorStatus::UpdateHealthStatus(
    std::uint32_t actor, ICombatActorStatusRuntime& runtime) {
    const auto current = runtime.QueryIndexed(actor, 0x34u, 7u);
    if (current <= 0) return runtime.Set(actor, 0xacu, 3);

    const auto maximum = runtime.QueryIndexed(actor, 0x30u, 0u);
    return runtime.Set(actor, 0xacu, CombatHealthState(current, maximum));
}

}  // namespace pal4
