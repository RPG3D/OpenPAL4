#include "pal4/actor_manager_combat_actor.h"

namespace pal4 {

std::uint32_t ActorManagerCombatActor::CleanupObjectLists(
    const std::uint32_t object,
    IActorManagerObjectListCleanupRuntime& runtime) {
    runtime.CleanupObjectLists(object);
    ActorManagerObjectPointerRange affected{};
    runtime.CollectObjects(object + 896u, affected);
    for (auto cursor = affected.begin; cursor != affected.end; cursor += 4u)
        runtime.HandleObjectFade(object, runtime.ReadWord(cursor), object);
    return runtime.Free(affected.begin);
}

}  // namespace pal4
