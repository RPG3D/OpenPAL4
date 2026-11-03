#include "pal4/script_npc_hand_effect.h"

namespace pal4 {

namespace {

std::uint32_t ResolveName(std::uint32_t payload,
                          IScriptNpcHandEffectRuntime& runtime) {
    const auto name = runtime.ReadWord(payload + 4u);
    return name == 0u ? runtime.EmptyStringAddress() : name;
}

std::uint32_t FindNpc(std::uint32_t payload,
                      IScriptNpcHandEffectRuntime& runtime) {
    const auto name = ResolveName(payload, runtime);
    return runtime.FindNpc(runtime.NpcManager(), name);
}

}  // namespace

std::uint32_t ScriptNpcHandEffect::Attach(
    std::uint32_t npc_name_object, std::uint32_t effect_name_object,
    std::uint8_t left_hand, IScriptNpcHandEffectRuntime& runtime) {
    const auto npc = FindNpc(npc_name_object, runtime);
    if (npc == 0u) return 0u;
    const auto effect = ResolveName(effect_name_object, runtime);
    return left_hand != 0u ? runtime.SetupLeftHandEffect(npc, effect) :
                             runtime.SetupRightHandEffect(npc, effect);
}

std::uint32_t ScriptNpcHandEffect::Detach(
    std::uint32_t npc_name_object, IScriptNpcHandEffectRuntime& runtime) {
    const auto npc = FindNpc(npc_name_object, runtime);
    return npc == 0u ? 0u : runtime.UpdateWeaponEffects(npc);
}

std::uint32_t ScriptNpcHandEffect::AttachGob(
    std::uint32_t gob_name_object, std::uint32_t render_name_object,
    std::uint32_t npc_name_object, std::uint8_t left_hand,
    IScriptNpcHandEffectRuntime& runtime) {
    const auto npc = FindNpc(npc_name_object, runtime);
    if (npc == 0u) return 0u;

    const auto length = runtime.ReadWord(gob_name_object + 8u);
    if (!runtime.NameDiffersFromEmptyError(gob_name_object, length)) {
        if (runtime.ReadWord(render_name_object + 8u) == 0u)
            return render_name_object;
        const auto name = ResolveName(render_name_object, runtime);
        const std::array<float, 3> unit_scale{1.0f, 1.0f, 1.0f};
        return runtime.RenderUnit(npc, 0u, unit_scale, name,
                                  left_hand != 0u);
    }

    const auto gob_name = ResolveName(gob_name_object, runtime);
    const auto object = runtime.FindObject(runtime.ObjectManager(), gob_name);
    if (object == 0u) return 0u;
    runtime.RemoveAnimatedObject(runtime.ArenaManager(), object);
    const auto render_name = ResolveName(render_name_object, runtime);
    const auto type = runtime.ReadWord(object + 0x14u);
    return runtime.RenderObject(npc, type, object + 0xb8u, render_name,
                                left_hand != 0u);
}

std::uint32_t ScriptNpcHandEffect::DetachGob(
    std::uint32_t npc_name_object, IScriptNpcHandEffectRuntime& runtime) {
    const auto npc = FindNpc(npc_name_object, runtime);
    return npc == 0u ? 0u : runtime.CleanupWeapons(npc);
}

}  // namespace pal4
