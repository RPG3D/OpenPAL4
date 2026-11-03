#include "pal4/actor_resource_loader.h"

namespace pal4 {

std::uint32_t ActorResourceLoader::Load(
    const std::uint32_t actor, const ActorLoadRequest& request,
    IActorResourceRuntime& runtime) {
    if (runtime.CombatInactive()) return 1;

    const auto existing_clump = runtime.ReadWord(actor + 20u);
    runtime.WriteByte(actor + 460u, 1u);
    const auto reload = !existing_clump ||
        !runtime.EqualIgnoringCase(
            runtime.TextAt(runtime.ReadWord(actor + 4u)), request.actor_name);
    if (reload) runtime.ResetActorProperties(actor);
    runtime.StartResourceContext(request.alternate_resource);
    if (reload)
        runtime.LoadComponentClump(actor, request.alternate_resource,
                                   request.actor_name);
    runtime.SetTexture(actor, request.texture_name);
    runtime.EndResourceContext();
    if (reload)
        runtime.UpdateAnimation(request.animation_context,
                                request.animation_name, request.animation_id);

    if (!runtime.ReadWord(actor + 20u))
        return runtime.CleanupComponent(actor) & 0xffu;

    (void)runtime.StoreDialogText(actor + 428u, request.dialog_text);
    runtime.StoreText(actor + 412u, request.primary_text);
    runtime.StoreText(actor + 444u, request.secondary_text);
    runtime.UpdateRenderState(actor);
    return 0;
}

std::uint8_t ActorPropertyReset::Reset(
    const std::uint32_t actor, IActorPropertyResetRuntime& runtime) {
    (void)runtime.StoreEmptyText(actor + 428u);
    (void)runtime.StoreEmptyText(actor + 412u);
    (void)runtime.StoreEmptyText(actor + 444u);
    runtime.RemoveEffect(actor + 396u);
    runtime.RemoveEffect(actor + 388u);
    runtime.HandleEffectEvent(actor + 404u, actor + 444u);
    const auto clump = runtime.ReadWord(actor + 20u);
    if (clump) {
        const auto frame = runtime.ReadWord(clump + 4u);
        if (frame) runtime.RemoveFrameChild(frame);
    }
    return runtime.CleanupComponent(actor);
}

}  // namespace pal4
