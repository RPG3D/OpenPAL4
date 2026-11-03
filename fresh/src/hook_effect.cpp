#include "pal4/hook_effect.h"

namespace pal4 {

std::uint32_t HookEffect::Create(
    std::uint32_t config, std::uint32_t clump,
    std::uint32_t context, IHookEffectFactoryRuntime& runtime) {
    if (clump == 0u) return 0u;
    const auto user_data = runtime.ClumpUserData(clump, "hook_effect");
    if (user_data == 0u) return 0u;
    const auto effect = runtime.Allocate(kObjectSize);
    if (effect == 0u) return 0u;
    runtime.WriteByte(effect + 4u, 0u);
    runtime.WriteWord(effect + 8u, 0u);
    runtime.WriteWord(effect, kVtable);
    if (runtime.Initialize(effect, config, user_data, context))
        return effect;
    runtime.Destroy(effect, true);
    return 0u;
}

double HookEffect::GetTotalTime(
    std::uint32_t effect, IHookEffectTimeRuntime& runtime) {
    const auto object = runtime.ReadWord(effect + 8u);
    runtime.WriteByte(effect + 4u, 1u);
    const auto frame = runtime.ReadWord(object + 44u);
    const auto transform = runtime.FrameTransform(frame);
    if (runtime.ReadWord(frame + 4u) != 0u)
        runtime.RemoveFrameChild(frame);
    runtime.SetFrameIdentity(frame);
    runtime.ApplyFrameTransform(frame, transform, 0u);
    runtime.UpdateFrameObjects(frame);
    return static_cast<double>(runtime.ReadFloat(object + 280u)) +
           static_cast<double>(runtime.ReadFloat(object + 276u));
}

}  // namespace pal4
