#include "pal4/actor_manager_player_monster_gate.h"

namespace pal4 {

std::int32_t ActorManagerPlayerMonsterGate::SelectAction(
    const std::uint32_t actor,
    IActorManagerPlayerMonsterGateRuntime& runtime) {
    const auto iterator = runtime.FirstIterator(runtime.SingletonManager());
    std::int32_t action = 0;
    try {
        bool found = false;
        while (!runtime.IteratorDone(iterator)) {
            const auto monster = runtime.IteratorValue(iterator);
            if (runtime.MonsterSlot412(monster)) {
                found = true;
                break;
            }
            runtime.IteratorAdvance(iterator);
        }
        if (found || runtime.CombatActive())
            action = runtime.ActorSlot536(actor);
        else
            action = runtime.ActorSlot532(actor);
    } catch (...) {
        if (iterator) (void)runtime.DestroyIterator(iterator, 1u);
        throw;
    }
    if (iterator) return runtime.DestroyIterator(iterator, 1u);
    return action;
}

}  // namespace pal4
