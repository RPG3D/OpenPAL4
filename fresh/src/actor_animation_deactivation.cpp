#include "pal4/actor_animation_control.h"

namespace pal4 {

void ActorAnimationControl::Deactivate(
    const std::uint32_t actor, const std::uint32_t effect_context,
    IActorCameraCleanupRuntime& runtime,
    IActorAttachmentCleanupRuntime& render_runtime) {
    runtime.CleanupCamera(actor);
    render_runtime.DeactivateAttachment(actor + 1088u, effect_context);
    render_runtime.DeactivateAttachment(actor + 624u, effect_context);
}

} // namespace pal4
