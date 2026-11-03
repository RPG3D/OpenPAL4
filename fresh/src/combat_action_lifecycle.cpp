#include "pal4/combat_action_lifecycle.h"

namespace pal4 {
namespace {

void CloseSelection(std::uint32_t event,
                    ICombatActionLifecycleRuntime& runtime) {
    const auto first_router = runtime.ScriptEventRouter();
    runtime.DispatchEvent(first_router, 0x8b8b78u, event, 1u);
    const auto second_router = runtime.ScriptEventRouter();
    runtime.DispatchEvent(second_router, 0x8b62d8u, event, 1u);
}

void RestoreEndingState(ICombatActionLifecycleRuntime& runtime) {
    const auto system = runtime.CombatSystem();
    if (runtime.IsCombatStateEnding(system)) {
        const auto update_system = runtime.CombatSystem();
        runtime.WriteWord(update_system + 508u, 2u);
    }
}

}  // namespace

std::uint8_t CombatActionLifecycle::ProcessExtended(
    std::uint32_t owner, std::uint32_t event,
    ICombatActionLifecycleRuntime& runtime) {
    const auto latch = runtime.ReadByte(owner + 72u);
    if (latch == 0u)
        return latch;

    const auto actor = runtime.ActorFromFacet(event);
    const auto manager = runtime.ActorManager();
    const auto iterator = runtime.CreateActorIterator(manager + 32u);
    std::uint8_t result = 0u;
    try {
        result = static_cast<std::uint8_t>(runtime.IteratorAtEnd(iterator));
        while (result == 0u) {
            const auto candidate = runtime.IteratorCurrent(iterator);
            if (candidate != actor && !runtime.CheckStateThree(candidate) &&
                runtime.IsStateZero(candidate) &&
                runtime.ActorSlot(candidate) >= 3) {
                const auto slot = runtime.ActorSlot(actor);
                if (runtime.IsActorReady(actor))
                    runtime.SetActorState(actor, 0u, 1u);
                runtime.FinalizeActor(actor);
                runtime.InitializeEffects(candidate, slot);
                const auto selected = runtime.ReadWord(owner + 68u);
                result = static_cast<std::uint8_t>(selected);
                if (selected != 0u) {
                    runtime.ResetActionComponent(selected + 716u);
                    CloseSelection(event, runtime);
                    runtime.WriteByte(owner + 72u, 0u);
                    RestoreEndingState(runtime);
                }
                break;
            }
            runtime.IteratorAdvance(iterator);
            result = static_cast<std::uint8_t>(runtime.IteratorAtEnd(iterator));
        }
    } catch (...) {
        if (iterator != 0u)
            (void)runtime.DestroyIterator(iterator);
        throw;
    }
    if (iterator != 0u)
        result = runtime.DestroyIterator(iterator);
    return result;
}

void CombatActionLifecycle::ProcessFinal(
    std::uint32_t owner, std::uint32_t event,
    ICombatActionLifecycleRuntime& runtime) {
    const auto actor = runtime.ReadWord(owner + 64u);
    if (actor == 0u || runtime.ReadByte(owner + 72u) == 0u ||
        runtime.ReadWord(actor + 804u) != 1u)
        return;
    if (runtime.GetOwnedChild(actor) == 0u)
        return;
    const auto child = runtime.GetOwnedChild(actor);
    const auto selected = runtime.ReadWord(owner + 64u);
    const auto task = runtime.CreateFollowupTask(child, selected);
    if (task == 0u)
        return;

    const auto manager = runtime.ActorManager();
    runtime.RegisterTask(manager + 96u, task);
    const auto reset_actor = runtime.ReadWord(owner + 64u);
    runtime.ResetActionComponent(reset_actor + 716u);
    CloseSelection(event, runtime);
    runtime.WriteByte(owner + 72u, 0u);
    runtime.WriteWord(owner + 64u, 0u);
    runtime.WriteWord(owner + 68u, 0u);
    RestoreEndingState(runtime);
}

std::uint32_t CombatActionLifecycle::SetStateToFive(
    ICombatActionLifecycleRuntime& runtime) {
    const auto system = runtime.CombatSystem();
    runtime.WriteWord(system + 508u, 5u);
    return system;
}

}  // namespace pal4
