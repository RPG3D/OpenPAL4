#include "pal4/script_object_effect.h"

namespace pal4 {
namespace {

std::uint32_t ResolveName(std::uint32_t payload,
                          IScriptObjectEffectRuntime& runtime) {
    const auto name = runtime.ReadWord(payload + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

}  // namespace

std::uint8_t ScriptObjectEffect::Play(
    std::uint32_t effect_name_payload, std::uint32_t argument,
    std::uint32_t object_name_payload,
    IScriptObjectEffectRuntime& runtime) {
    const auto object_name = ResolveName(object_name_payload, runtime);
    const auto object = runtime.FindByName(runtime.ObjectManager(),
                                           object_name);
    if (object == 0u) return 0u;

    const auto active = object + 0x1ccu;
    const auto loaded = object + 0x1d4u;
    if (runtime.NeedsUpdate(active) != 0u) runtime.UpdateContent(active);
    if (runtime.IsReady(loaded) != 0u)
        (void)runtime.DisplayAndCleanup(loaded);

    const auto effect_name = ResolveName(effect_name_payload, runtime);
    const auto type = runtime.ReadWord(object + 0x14u);
    const auto loaded_handle = runtime.LoadFromXml(
        runtime.EffectManager(), effect_name, type);
    runtime.WriteHandle(loaded, loaded_handle);
    const auto ready = runtime.IsReady(loaded);
    if (ready != 0u) return ready;

    const auto fallback_name = ResolveName(effect_name_payload, runtime);
    auto handle = runtime.CreateAtAddress(
        runtime.EffectManager(), fallback_name, object + 0xacu,
        argument, 0u);
    runtime.WriteHandle(active, handle);
    return runtime.ResetHandle(handle);
}

std::uint8_t ScriptObjectEffect::UpdateAndShow(
    std::uint32_t object_name_payload,
    IScriptObjectEffectRuntime& runtime) {
    const auto object_name = ResolveName(object_name_payload, runtime);
    const auto object = runtime.FindByName(runtime.ObjectManager(),
                                           object_name);
    if (object == 0u) return 0u;

    const auto active = object + 0x1ccu;
    if (runtime.NeedsUpdate(active) != 0u) runtime.UpdateContent(active);
    const auto loaded = object + 0x1d4u;
    const auto ready = runtime.IsReady(loaded);
    if (ready == 0u) return ready;
    return runtime.DisplayAndCleanup(loaded);
}

}  // namespace pal4
