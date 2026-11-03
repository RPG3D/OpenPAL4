#include "pal4/combat_extended_actor_callbacks.h"

namespace pal4 {

std::uint32_t CombatExtendedActorCallbacks::ProcessActorEvent(
    std::uint32_t owner, std::uint32_t event,
    ICombatPlayerReadyRuntime& runtime) {
    const auto candidate = runtime.ActorFromFacet(event);
    if (runtime.ProcessReadyCandidate(candidate)) {
        runtime.WriteWord(owner + 64u, runtime.ActorFromFacet(event));
        const auto system = runtime.CombatSystem();
        if (!runtime.IsCombatSystemInactive(system)) {
            const auto first = runtime.ActorFromFacet(event);
            if (runtime.ActorState(first) != 5u) {
                const auto second = runtime.ActorFromFacet(event);
                if (runtime.ActorState(second) != 6u) {
                    const auto update_system = runtime.CombatSystem();
                    runtime.WriteWord(update_system + 508u, 4u);
                    runtime.ProcessActor(runtime.ActorFromFacet(event), 1u);
                    runtime.WriteWord(owner + 68u,
                                      runtime.ActorFromFacet(event));
                    const auto router = runtime.ScriptEventRouter();
                    const auto result = runtime.ProcessActorEvent(
                        router, 0x8b62d4u, event, 1u);
                    runtime.WriteByte(owner + 72u, 1u);
                    return result;
                }
            }
        }
        runtime.FinalizeActor(runtime.ActorFromFacet(event));
    }
    const auto actor = runtime.ActorFromFacet(event);
    return runtime.ResetActionComponent(actor + 716u);
}

std::uint32_t CombatExtendedActorCallbacks::HandleActorCombatEvent(
    std::uint32_t event, ICombatMonsterReadyRuntime& runtime) {
    const auto first_system = runtime.CombatSystem();
    if (static_cast<std::int32_t>(
            runtime.ReadWord(first_system + 432u)) > 0) {
        const auto actor = runtime.ActorFromFacet(event);
        if (runtime.ReadWord(actor + 804u) == 2u) {
            const auto id_system = runtime.CombatSystem();
            const auto id = runtime.ActorId(actor);
            if (id == runtime.ReadWord(id_system + 436u)) {
                const auto secondary_system = runtime.CombatSystem();
                const auto secondary = runtime.ActorSecondaryId(actor);
                if (secondary ==
                    runtime.ReadWord(secondary_system + 432u)) {
                    const auto result_system = runtime.CombatSystem();
                    const auto value = runtime.ReadWord(result_system + 448u);
                    const auto router = runtime.ScriptEventRouter();
                    return runtime.ProcessValueEvent(router, 0x8b8b8cu,
                                                     value, 1u);
                }
            }
        }
    }
    const auto candidate = runtime.ActorFromFacet(event);
    if (runtime.ProcessReadyCandidate(candidate))
        runtime.FinalizeActor(runtime.ActorFromFacet(event));
    const auto actor = runtime.ActorFromFacet(event);
    return runtime.ResetActionComponent(actor + 716u);
}

}  // namespace pal4
