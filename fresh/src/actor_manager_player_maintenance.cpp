#include "pal4/actor_manager_player_maintenance.h"

#include <cstddef>

namespace pal4 {

bool ActorManagerPlayerMaintenance::ProcessTask(
    const std::uint32_t actor,
    IActorManagerPlayerMaintenanceRuntime& runtime) {
    return runtime.ProcessAIActionTask(runtime.ReadWord(actor + 1288u));
}

std::uint32_t ActorManagerPlayerMaintenance::CleanupData(
    const std::uint32_t actor,
    IActorManagerPlayerMaintenanceRuntime& runtime) {
    runtime.CleanupObjectLists(actor);
    std::vector<std::uint32_t> items;
    items.reserve(32u);
    runtime.ProcessInventory(actor + 896u, items);
    runtime.ProcessIndexedCollection(actor + 920u, items);
    runtime.ProcessSortedValues(actor + 920u, items);
    for (std::size_t i = 0u; i < items.size(); ++i)
        runtime.HandleGameObjectFade(actor, items[i], actor, items);
    return 0u;
}

}  // namespace pal4
