#include "pal4/actor_manager_combat_actor_binding.h"

namespace pal4 {

namespace {
class ImageAttachment final : public IActorModelAttachmentRuntime {
public:
    IActorManagerCombatBindingRuntime& r;
    explicit ImageAttachment(IActorManagerCombatBindingRuntime& r):r(r){}
    std::uint32_t Model(std::uint32_t actor) override { return r.ReadWord(actor+808u); }
    void StoreModel(std::uint32_t actor,std::uint32_t child) override { r.WriteWord(actor+808u,child); }
    void DetachCurrent(std::uint32_t actor) override { r.ActorSlot588(actor); }
    std::uint32_t AcquireModel(std::string_view name) override {
        const auto collection=r.ManagerSingleton()+64u;
        return r.CreateChild(collection,collection+16u,name);
    }
    void InitializeFrame(std::uint32_t child) override { r.GetAnimationFrame(child); }
    void DisableCamera(std::uint32_t child) override { r.RenderCamera(child,0u,0u); }
    std::uint32_t BindParent(std::uint32_t child,std::uint32_t actor) override {
        return r.InitializeChild(child,actor);
    }
    void ReturnModel(std::uint32_t child,std::uint32_t limit) override {
        const auto collection=r.ManagerSingleton()+64u;
        r.ReleaseChild(collection,collection+16u,limit,child);
    }
};
}

bool ActorManagerCombatActorBinding::IsActionValid(
    const std::uint32_t actor, IActorManagerCombatBindingRuntime& runtime) {
    return runtime.ProcessActionTask(runtime.ReadWord(actor + 1156u));
}

std::uint32_t ActorManagerCombatActorBinding::Value924(
    const std::uint32_t actor, const std::uint32_t,
    IActorManagerCombatValueRuntime& runtime) {
    return runtime.ReadWord(actor + 924u);
}

std::uint32_t ActorManagerCombatActorBinding::AttachChild(
    const std::uint32_t actor, const std::string_view name,
    IActorManagerCombatBindingRuntime& runtime) {
    ImageAttachment adapter(runtime);
    return AttachChild(actor,name,adapter);
}

void ActorManagerCombatActorBinding::DetachChild(
    const std::uint32_t actor, IActorManagerCombatBindingRuntime& runtime) {
    ImageAttachment adapter(runtime);
    DetachChild(actor,adapter);
}

std::uint32_t ActorManagerCombatActorBinding::SetValue1164(
    const std::uint32_t actor, const std::uint32_t value,
    IActorManagerCombatWordRuntime& runtime) {
    runtime.WriteWord(actor + 1164u, value);
    return value;
}

}  // namespace pal4
