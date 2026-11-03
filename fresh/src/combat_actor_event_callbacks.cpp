#include "pal4/combat_actor_event_callbacks.h"

namespace pal4 {
namespace {

constexpr std::uint32_t kCombatOverDescriptor = 0x8b8b8cu;

std::uint32_t Dispatch(std::uint32_t payload,
                       ICombatActorActionEndRuntime& runtime) {
    const auto router = runtime.ScriptEventRouter();
    return runtime.ProcessScriptEvent(router, kCombatOverDescriptor,
                                      payload, 1u);
}

}  // namespace

bool CombatActorEventCallbacks::HandleActorEvent(
    std::uint32_t actor, ICombatActorReplacementRuntime& runtime) {
    if (runtime.ActorSlot(actor) >= 6) {
        runtime.RetireReplacementActor(actor);
        return false;
    }

    const auto iterator = runtime.CreateCandidateIterator(actor);
    bool transferred = false;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto candidate = runtime.IteratorCurrent(iterator);
            if (runtime.IsStateZero(candidate) && candidate != actor) {
                const auto slot = runtime.ActorSlot(actor);
                runtime.RetireReplacementActor(actor);
                runtime.InitializeEffects(candidate, slot);
                transferred = true;
                break;
            }
            runtime.IteratorAdvance(iterator);
        }
    } catch (...) {
        runtime.DestroyIterator(iterator);
        throw;
    }
    runtime.DestroyIterator(iterator);
    return transferred;
}

std::uint32_t CombatActorEventCallbacks::GetRotationAndProcess(
    std::uint32_t event, ICombatActorEventRuntime& runtime) {
    const auto first = runtime.ActorFromFacet(event);
    const auto rotation = runtime.ActorRotation(first, 0u);
    const auto second = runtime.ActorFromFacet(event);
    runtime.ProcessChildSlot4(second, rotation, 0u);
    const auto third = runtime.ActorFromFacet(event);
    return runtime.ProcessRotationResult(third, 1u);
}

void CombatActorEventCallbacks::HandleStateChange(
    std::uint32_t event, ICombatActorEventRuntime& runtime) {
    if (runtime.ReadWord(event + 4u) != 4u)
        return;
    const auto actor = runtime.ActorFromFacet(event);
    if (runtime.ReadWord(actor + 804u) != 2u ||
        !runtime.CheckStateThree(actor))
        return;
    const auto system = runtime.CombatSystem();
    if (runtime.ReadWord(system + 444u) == 0u)
        return;
    const auto current_system = runtime.CombatSystem();
    const auto actor_id = runtime.ActorId(actor);
    if (actor_id == runtime.ReadWord(current_system + 444u)) {
        const auto update_system = runtime.CombatSystem();
        runtime.WriteWord(update_system + 508u, 6u);
    }
}

std::uint32_t CombatActorEventCallbacks::HandleExtended(
    std::uint32_t owner, std::uint32_t event,
    ICombatActorActionEndRuntime& runtime) {
    const auto actor = runtime.ActorFromFacet(event);
    const auto system = runtime.CombatSystem();
    if (runtime.IsCombatStateActive(system)) {
        const auto iterator = runtime.CreateExtendedIterator(actor);
        try {
            if (!runtime.IteratorAtEnd(iterator)) {
                while (true) {
                    const auto current = runtime.IteratorCurrent(iterator);
                    if (runtime.ObjectStateNotThree(current)) {
                        if (runtime.ActorVisualMode(actor) == 1u)
                            runtime.SetVisualScalar(actor + 716u,
                                                    0x42000000u);
                        break;
                    }
                    runtime.IteratorAdvance(iterator);
                    if (runtime.IteratorAtEnd(iterator))
                        break;
                }
            }
        } catch (...) {
            runtime.DestroyIterator(iterator);
            throw;
        }
        runtime.DestroyIterator(iterator);
    }

    const auto result = runtime.CheckActorStateExtended(owner);
    if (result != 3u) {
        if (runtime.ReadByte(owner + 136u) != 0u)
            return Dispatch(result, runtime);
        runtime.WriteByte(owner + 136u, 1u);
    }
    return result;
}

std::uint8_t CombatActorEventCallbacks::HandleFled(std::uint32_t event,ICombatActorFledRuntime& runtime){
    const auto actor=runtime.ActorFromFacet(event);
    if(runtime.ReadWord(actor+804u)==1u)return static_cast<std::uint8_t>(runtime.DispatchResult(2u));
    if(runtime.IsLastActor(actor)){
        runtime.SetActorState(actor,6u,1u);(void)runtime.FinalizeActor(actor);
        return static_cast<std::uint8_t>(runtime.DispatchResult(1u));
    }
    if(runtime.ReplaceActor(actor))return 1u;
    return runtime.FinalizeActor(actor);
}
std::uint8_t CombatActorEventCallbacks::HandleType(
    [[maybe_unused]] std::uint32_t owner,std::uint32_t event,ICombatActorEventRuntime& original){
    class Runtime final:public ICombatActorFledRuntime {
        ICombatActorEventRuntime& r;
    public:
        explicit Runtime(ICombatActorEventRuntime& r):r(r){}
        std::uint32_t ActorFromFacet(std::uint32_t e)override{return r.ActorFromFacet(e);}
        std::uint32_t ReadWord(std::uint32_t a)override{return r.ReadWord(a);}
        bool IsLastActor(std::uint32_t a)override{return CombatActorEventCallbacks::CheckActorState(a,r);}
        void SetActorState(std::uint32_t a,std::uint32_t s,std::uint32_t m)override{r.SetActorState(a,s,m);}
        std::uint8_t FinalizeActor(std::uint32_t a)override{return r.FinalizeActor(a);}
        bool ReplaceActor(std::uint32_t a)override{return CombatActorEventCallbacks::HandleActorEvent(a,r);}
        std::uint32_t DispatchResult(std::uint32_t result)override{return Dispatch(result,r);}
    } runtime(original);
    return HandleFled(event,runtime);
}

bool CombatActorEventCallbacks::CheckActorState(
    std::uint32_t actor, ICombatActorFirstRosterRuntime& runtime) {
    const auto iterator = runtime.CreateCandidateIterator(actor);
    bool found = false;
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto candidate = runtime.IteratorCurrent(iterator);
            if (candidate != actor &&
                (runtime.IsActorReady(candidate) ||
                 runtime.IsStateZero(candidate))) {
                found = true;
                break;
            }
            runtime.IteratorAdvance(iterator);
        }
    } catch (...) {
        runtime.DestroyIterator(iterator);
        throw;
    }
    runtime.DestroyIterator(iterator);
    return !found;
}

}  // namespace pal4
