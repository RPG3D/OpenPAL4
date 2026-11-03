#include "pal4/actor_animation_binding.h"

namespace pal4 {
namespace {

constexpr auto kPrimaryAxis = 0x84493cu;
constexpr auto kSecondaryAxis = 0x844948u;

std::uint32_t Rotate(const std::uint32_t hook, const bool left_weapon,
                     const bool alternate,
                     IActorAnimationBindingRuntime& runtime) {
    const auto clump = runtime.ReadWord(hook + 60u);
    if (!clump) return hook;
    auto callback = runtime.IdentityRenderCallback();
    if (!callback) callback = runtime.DefaultRenderCallback();
    runtime.WriteWord(hook + 72u, callback);
    const auto frame = runtime.ReadWord(clump + 4u);
    const auto source = runtime.ReadWord(hook + 4u);
    runtime.InvertMatrix(frame + 16u, source + 16u);
    runtime.UpdateFrame(frame);
    const auto updated_frame = runtime.ReadWord(clump + 4u);
    const auto angle = alternate && left_weapon ? -90.0f : 90.0f;
    runtime.RotateMatrix(updated_frame + 16u, kPrimaryAxis, angle, 2u);
    if (!left_weapon)
        runtime.RotateMatrix(updated_frame + 16u, kSecondaryAxis,
                             180.0f, 2u);
    runtime.UpdateFrame(updated_frame);
    return 0;
}

}  // namespace

std::uint32_t ActorAnimationBinding::RotateOrdinary(
    const std::uint32_t hook, const bool left_weapon,
    IActorAnimationBindingRuntime& runtime) {
    return Rotate(hook, left_weapon, false, runtime);
}

std::uint32_t ActorAnimationBinding::RotateAlternate(
    const std::uint32_t hook, const bool left_weapon,
    IActorAnimationBindingRuntime& runtime) {
    return Rotate(hook, left_weapon, true, runtime);
}

std::uint32_t ActorAnimationBinding::BindCallback(
    const std::uint32_t actor, const bool left_weapon,
    const std::uint32_t mode, IActorAnimationBindingRuntime& runtime) {
    const auto clump = runtime.ReadWord(actor + 20u);
    if (!clump) return 0;
    const auto hook = runtime.FindHookHand(clump);
    if (!hook) return 0;
    return mode == 1u ? RotateAlternate(hook, left_weapon, runtime)
                      : RotateOrdinary(hook, left_weapon, runtime);
}

std::uint32_t ActorAnimationBinding::Update(
    const std::uint32_t actor, const std::uint32_t context,
    const std::string_view name, const std::uint32_t mode,
    IActorAnimationBindingRuntime& runtime) {
    const auto left_weapon = runtime.EqualIgnoringCase(
        name, "hook_LHWeapon");
    (void)BindCallback(actor, left_weapon, mode, runtime);
    const auto clump = runtime.ReadWord(actor + 20u);
    const auto frame = runtime.ReadWord(clump + 4u);
    runtime.ProcessUserData(context, name, frame);
    return runtime.HideHookAtomics(runtime.ReadWord(actor + 20u));
}

}  // namespace pal4
