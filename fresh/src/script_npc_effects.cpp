#include "pal4/script_npc_effects.h"

namespace pal4 {

std::uint32_t ScriptNpcEffects::Attach(
    const std::uint32_t name_record,
    const std::uint32_t effect_record,
    const std::int32_t argument,
    IScriptNpcEffectsRuntime& runtime) {
    auto name = runtime.ReadWord(name_record + 4u);
    if (name == 0u) name = runtime.EmptyString();
    const auto npc = runtime.FindNpc(runtime.NpcManager(), name);
    if (npc == 0u) return 0u;
    auto effect = runtime.ReadWord(effect_record + 4u);
    if (effect == 0u) effect = runtime.EmptyString();
    return runtime.AttachEffect(npc, effect, argument);
}

std::uint32_t ScriptNpcEffects::Detach(
    const std::uint32_t name_record,
    IScriptNpcEffectsRuntime& runtime) {
    auto name = runtime.ReadWord(name_record + 4u);
    if (name == 0u) name = runtime.EmptyString();
    const auto npc = runtime.FindNpc(runtime.NpcManager(), name);
    if (npc == 0u) return 0u;
    return runtime.DetachEffect(npc);
}

}  // namespace pal4
