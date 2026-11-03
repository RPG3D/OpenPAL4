#include "pal4/actor_instance_animation.h"

#include <string>

namespace pal4 {

std::uint32_t ActorInstanceAnimation::GetFrame(
    const std::uint32_t actor, IActorInstanceFrameRuntime& runtime) {
    runtime.SetAnimationFlag(actor + 368u, 0);
    runtime.SetAnimationName(actor + 368u, runtime.GlobalAnimationName());
    runtime.InvokeActorSlot24(actor, runtime.ReadWord(actor + 520u));
    return runtime.InvokeActorSlot20(actor);
}

bool ActorInstanceAnimation::Play(
    const std::uint32_t actor, const std::string_view resource_name,
    const std::string_view action_name,
    IActorInstanceAnimationRuntime& runtime) {
    runtime.BeginResourceContext();
    runtime.SetResourceName(actor + 496u, resource_name);
    std::string directory(runtime.PathPrefixOne());
    directory += runtime.PathPrefixTwo();
    directory += action_name;
    directory += '\\';
    const auto game = runtime.GameInstance();
    if (!runtime.LoadClump(actor, resource_name, directory,
                           action_name, game + 428u)) return false;

    const auto clump = runtime.ReadWord(actor + 20u);
    runtime.LoadAnimationData(actor + 368u, clump, directory, action_name);
    for (auto index = 0u; index < 4u; ++index)
        runtime.WriteWord(actor + 608u + index * 4u,
                          runtime.ReadWord(actor + 208u + index * 4u));
    const auto shadow_game = runtime.GameInstance();
    if (runtime.LoadShadowClump(actor, shadow_game + 428u))
        runtime.AttachHierarchy(actor + 368u, runtime.ReadWord(actor + 48u));
    runtime.InitializeBoundingBox(actor + 524u, clump);
    runtime.HideHookAtomics(runtime.ReadWord(actor + 20u));
    runtime.WriteWord(actor + 516u, runtime.ReadWord(actor + 492u));
    runtime.WriteWord(actor + 412u, actor + 512u);
    runtime.WriteWord(actor + 520u, 0xffffffffu);
    runtime.EndResourceContext();
    return true;
}

void ActorInstanceAnimation::Render(
    const std::uint32_t actor, const std::uint32_t argument,
    IActorInstanceRenderRuntime& runtime) {
    runtime.RenderClump(actor, argument);
    runtime.ProcessAnimation(actor + 368u, argument);
    runtime.RenderChild(actor + 624u, argument);
    runtime.RenderChild(actor + 1088u, argument);
}

std::uint32_t ActorInstanceAnimation::Update(
    const std::uint32_t actor, const std::uint32_t elapsed_bits,
    IActorInstanceUpdateRuntime& runtime) {
    runtime.UpdateAndRenderClump(actor, elapsed_bits);
    runtime.UpdateInterpolator(actor + 368u, elapsed_bits);
    return runtime.UpdateBoundingBox(actor + 524u,
                                     runtime.ReadWord(actor + 20u));
}

}  // namespace pal4
