#include "pal4/actor_manager_combat_actor_binding.h"

namespace pal4 {
std::uint32_t ActorManagerCombatActorBinding::AttachChild(std::uint32_t actor,
    std::string_view name,IActorModelAttachmentRuntime& runtime) {
    runtime.DetachCurrent(actor);
    const auto child=runtime.AcquireModel(name);
    runtime.StoreModel(actor,child);
    if (!child) return 0u;
    runtime.InitializeFrame(child);
    runtime.DisableCamera(child);
    return runtime.BindParent(child,actor);
}
void ActorManagerCombatActorBinding::DetachChild(std::uint32_t actor,
    IActorModelAttachmentRuntime& runtime) {
    const auto child=runtime.Model(actor);
    if (!child) return;
    runtime.ReturnModel(child,12u);
    runtime.StoreModel(actor,0u);
}
} // namespace pal4
