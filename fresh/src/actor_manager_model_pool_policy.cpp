#include "pal4/actor_manager_actor_pool.h"

namespace pal4 {
std::uint32_t ActorManagerActorPool::TakeOrCreate(
    std::string_view name, IActorManagerModelPoolRuntime& runtime) {
    if (runtime.CombatInactive()) return 0u;
    const auto count=runtime.CachedCount();
    for (std::size_t i=0;i<count;++i)
        if (runtime.CachedName(i)==name) return runtime.ExtractCached(i);
    return runtime.CreateModel(name);
}
void ActorManagerActorPool::Return(
    std::uint32_t limit, std::uint32_t actor, std::uint32_t context,
    IActorManagerModelPoolRuntime& runtime) {
    if (!actor) return;
    runtime.DeactivateModel(actor,context);
    if (runtime.CachedCount()>=limit) runtime.EvictOldest();
    runtime.AppendModel(actor);
}
} // namespace pal4
