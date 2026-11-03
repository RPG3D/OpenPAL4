#include "pal4/script_monster_effects.h"

namespace pal4 {

std::uint32_t ScriptMonsterEffects::Attach(
    const std::uint32_t name_record,
    const std::uint32_t effect_record,
    const std::int32_t argument,
    IScriptMonsterEffectsRuntime& runtime) {
    auto name = runtime.ReadWord(name_record + 4u);
    if (name == 0u) name = runtime.EmptyString();
    const auto monster = runtime.FindMonster(runtime.MonsterManager(), name);
    if (monster == 0u) return 0u;
    auto effect = runtime.ReadWord(effect_record + 4u);
    if (effect == 0u) effect = runtime.EmptyString();
    return runtime.AttachEffect(monster, effect, argument);
}

std::uint32_t ScriptMonsterEffects::Detach(
    const std::uint32_t name_record,
    IScriptMonsterEffectsRuntime& runtime) {
    auto name = runtime.ReadWord(name_record + 4u);
    if (name == 0u) name = runtime.EmptyString();
    const auto monster = runtime.FindMonster(runtime.MonsterManager(), name);
    if (monster == 0u) return 0u;
    return runtime.DetachEffect(monster);
}

}  // namespace pal4
