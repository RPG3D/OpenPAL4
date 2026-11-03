#include "pal4/actor_animation_control.h"

namespace pal4 {

bool ActorAnimationControl::FindAnimation(
    const std::uint32_t actor, const std::uint32_t search,
    IActorAnimationSearchRuntime& runtime) {
    runtime.SetAnimationFound(false);
    runtime.ProcessAnimations(actor + 368u, search,
        [&](const std::uint32_t object, const std::uint32_t key) {
            return ActorAnimationEvents::CompareNameAndFlag(object, key,
                                                             runtime);
        });
    return runtime.AnimationFound();
}


namespace {
std::uint32_t LoadHand(
    const std::uint32_t actor, const ActorAttachmentLoadArgs& args,
    const std::uint32_t hand_offset, const std::string_view animation_name,
    IActorResourceRuntime& runtime) {
    //52AD50/52ADA0 keep the body clump as animation context, then adjust
    //the receiver to the embedded hand before entering529D00.
    return ActorResourceLoader::Load(actor + hand_offset, {
        runtime.ReadWord(actor + 20u), animation_name, args.model_name,
        args.texture_name, args.animation_id, args.primary_text,
        args.dialog_text, args.secondary_text, args.alternate_resource
    }, runtime);
}
}  // namespace

std::uint32_t ActorAnimationControl::LoadRightHand(
    const std::uint32_t actor, const ActorAttachmentLoadArgs& args,
    IActorResourceRuntime& runtime) {
    return LoadHand(actor, args, 1088u, "hook_RHWeapon", runtime);
}


std::uint32_t ActorAnimationControl::LoadLeftHand(
    const std::uint32_t actor, const ActorAttachmentLoadArgs& args,
    IActorResourceRuntime& runtime) {
    return LoadHand(actor, args, 624u, "hook_LHWeapon", runtime);
}


}  // namespace pal4
