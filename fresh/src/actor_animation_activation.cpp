#include "pal4/actor_animation_control.h"

namespace pal4 {

void ActorAnimationControl::Activate(
    const std::uint32_t actor, const std::uint32_t camera_argument,
    const std::uint32_t atomic_callback,
    const std::uint32_t effect_context,
    IActorAnimationControlRuntime& runtime,
    IActorRenderStateRuntime& render_runtime) {
    const auto clump = runtime.ReadWord(actor + 20u);
    if (clump)
        (void)runtime.ProcessClumpAtomics(clump, atomic_callback);
    if (LeftHandClump(actor, runtime))
        (void)runtime.ProcessClumpAtomics(
            LeftHandClump(actor, runtime), atomic_callback);
    if (RightHandClump(actor, runtime))
        (void)runtime.ProcessClumpAtomics(
            RightHandClump(actor, runtime), atomic_callback);
    runtime.RenderCamera(actor, camera_argument);
    SetLeftHandActive(actor, 1u, effect_context, render_runtime);
    SetRightHandActive(actor, 1u, effect_context, render_runtime);
}

void ActorAnimationControl::SetLeftHandActive(
    const std::uint32_t actor, const std::uint8_t active,
    const std::uint32_t effect_context,
    IActorRenderStateRuntime& runtime) {
    runtime.SetAttachmentState(actor + 624u, active,effect_context);
}

void ActorAnimationControl::SetRightHandActive(
    const std::uint32_t actor, const std::uint8_t active,
    const std::uint32_t effect_context,
    IActorRenderStateRuntime& runtime) {
    runtime.SetAttachmentState(actor + 1088u, active,effect_context);
}

std::uint32_t ActorAnimationControl::RightHandClump(
    const std::uint32_t actor, IActorAnimationControlRuntime& runtime) {
    return runtime.ReadWord(actor + 1108u);
}

std::uint32_t ActorAnimationControl::LeftHandClump(
    const std::uint32_t actor, IActorAnimationControlRuntime& runtime) {
    return runtime.ReadWord(actor + 644u);
}

}  // namespace pal4
