#include "pal4/combat_event_processor.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kEventDescriptor = 0x8b8b8cu;

std::uint8_t Dispatch(std::uint32_t payload,
                      ICombatEventProcessRuntime& runtime) {
    const auto router = runtime.ScriptEventRouter();
    return runtime.ProcessScriptEvent(router, kEventDescriptor, payload, 1u);
}

void ProcessActorCollection(std::uint32_t actor,
                            ICombatEventProcessRuntime& runtime) {
    const auto iterator = runtime.CreateActorIterator(actor);
    std::uint32_t pending = 0u;
    try {
        if (!runtime.IteratorAtEnd(iterator)) {
            while (true) {
                const auto current = runtime.IteratorCurrent(iterator);
                if (runtime.IsActorReady(current))
                    break;
                const auto candidate = runtime.IteratorCurrent(iterator);
                if (!runtime.ActorCondition(candidate))
                    pending = runtime.IteratorCurrent(iterator);
                runtime.IteratorAdvance(iterator);
                if (runtime.IteratorAtEnd(iterator)) {
                    if (pending != 0u)
                        (void)runtime.HandleActorEvent(pending);
                    break;
                }
            }
        }
    } catch (...) {
        runtime.DestroyIterator(iterator);
        throw;
    }
    runtime.DestroyIterator(iterator);
}

}  // namespace

std::uint8_t CombatEventProcessor::Process(
    std::uint32_t owner, std::uint32_t event,
    ICombatEventProcessRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    const auto type = runtime.ReadWord(actor + 804u);
    if (type == 2u) {
        const auto system = runtime.CombatSystem();
        if (runtime.ReadWord(system + 444u) != 0u) {
            const auto current_system = runtime.CombatSystem();
            const auto actor_id = runtime.ActorId(actor);
            if (actor_id == runtime.ReadWord(current_system + 444u)) {
                const auto update_system = runtime.CombatSystem();
                runtime.WriteWord(update_system + 508u, 3u);
                return Dispatch(1u, runtime);
            }
        }
        if (!runtime.HandleActorEvent(actor))
            runtime.FinalizeActor(actor);
        if (static_cast<std::int32_t>(runtime.ReadWord(actor + 964u)) > 0) {
            runtime.GameInstance();
            runtime.HandleActorProgress(runtime.ActorId(actor));
        }
    } else {
        ProcessActorCollection(actor, runtime);
    }

    const auto status = runtime.ObjectStatus(actor);
    if (status != 0u)
        return status;
    const auto actor_state = runtime.CheckActorState(actor);
    if (actor_state == 0u)
        return actor_state;
    if (runtime.ReadByte(owner + 136u) == 0u) {
        const auto ready = runtime.CheckActorsReady();
        if (ready != 0u) {
            runtime.WriteByte(owner + 136u, 1u);
            return ready;
        }
    }
    const auto system = runtime.CombatSystem();
    const auto active = runtime.IsCombatStateActive(system);
    if (active != 0u)
        return Dispatch(runtime.ReadWord(actor + 804u) == 2u ? 1u : 0u,
                        runtime);
    return active;
}

}  // namespace pal4
