#include "pal4/combat_action_registry.h"

namespace pal4 {

std::uint32_t CombatActionRegistry::RegisterActorEvent(
    const std::uint32_t event, ICombatActionRegistryRuntime& runtime) {
    const auto manager = runtime.ActorManagerSingleton();
    if (manager == 0u) return 0u;
    return ScriptTaskQueue::RegisterEvent(manager + 96u, event, runtime);
}

}  // namespace pal4
