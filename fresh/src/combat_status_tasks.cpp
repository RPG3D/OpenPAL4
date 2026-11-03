#include "pal4/combat_status_tasks.h"

namespace pal4 {

std::uint8_t CombatStatusTasks::Defend(
    const std::uint32_t task, ICombatStatusTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (actor) {
        runtime.CallActor(actor, 540u, 0u);
        runtime.SetStateOne(runtime.ReadWord(task + 60u));
    }
    return 1u;
}

std::uint8_t CombatStatusTasks::Flee(
    const std::uint32_t task, ICombatStatusTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (!actor) return 0u;
    runtime.CallActor(actor, 528u, 0u);
    return 1u;
}

std::uint8_t CombatStatusTasks::SetFleeTarget(
    const std::uint32_t task, ICombatStatusTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (!actor) return 0u;
    runtime.CallActor(actor, runtime.ReadByte(task + 64u) ? 532u : 536u, 0u);
    return 1u;
}

std::uint8_t CombatStatusTasks::Dead(
    const std::uint32_t task, ICombatStatusTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (!actor) return 0u;
    const auto target = runtime.ReadWord(task + 64u);
    if (!target) return 0u;
    runtime.CallActor(actor, 544u, target);
    return 1u;
}

std::uint8_t CombatStatusTasks::FendOff(
    const std::uint32_t task, ICombatStatusActorRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (!actor) return 0u;
    const auto target = runtime.ReadWord(task + 64u);
    if (!target) return 0u;
    runtime.CallActor(actor, 508u, target);
    return 1u;
}

}  // namespace pal4
