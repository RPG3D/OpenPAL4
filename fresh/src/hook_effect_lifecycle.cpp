#include "pal4/hook_effect.h"

namespace pal4 {

void HookEffect::CleanupObject(
    std::uint32_t effect, IHookEffectLifecycleRuntime& runtime) {
    auto object = runtime.ReadWord(effect + 8u);
    if (object == 0u) return;
    runtime.ResetParticleFields(object);
    object = runtime.ReadWord(effect + 8u);
    if (object == 0u) return;
    runtime.DestroyParticleObject(object);
    runtime.WriteWord(effect + 8u, 0u);
}

std::uint32_t HookEffect::UpdateState(
    std::uint32_t effect, double seconds,
    IHookEffectLifecycleRuntime& runtime) {
    const auto object = runtime.ReadWord(effect + 8u);
    if (object == 0u) return effect;
    const auto owner = runtime.ReadWord(effect + 12u);
    return runtime.UpdateParticle(
        object, seconds, runtime.ReadWord(owner + 48u),
        runtime.ReadByte(effect + 4u));
}

std::optional<std::uint32_t> HookEffect::HandleStateChange(
    std::uint32_t effect, std::uint32_t state,
    IHookEffectLifecycleRuntime& runtime) {
    const auto object = runtime.ReadWord(effect + 8u);
    if (object == 0u) return std::nullopt;
    return runtime.HandleParticleStateChange(object, state);
}

void HookEffect::Destruct(
    std::uint32_t effect, IHookEffectLifecycleRuntime& runtime) {
    runtime.WriteWord(effect, kVtable);
    CleanupObject(effect, runtime);
    DestructBase(effect, runtime);
}

void HookEffect::DestructBase(
    std::uint32_t effect, IHookEffectLifecycleRuntime& runtime) {
    runtime.WriteWord(effect, 0x84b208u);
}

}  // namespace pal4
