#include "pal4/script_combat_vip_monster.h"

namespace pal4 {

std::uint32_t ScriptCombatVipMonster::Configure(
    const std::uint32_t monster_id,
    IScriptCombatVipMonsterRuntime& runtime) {
    const auto combat = runtime.CombatSystem();
    CombatConfigSnapshot config;
    for (std::uint32_t i = 0u; i < config.words.size(); ++i)
        config.words[i] = runtime.ReadWord(combat + 0x1b0u + i * 4u);

    std::uint32_t copied_texts = 0u;
    try {
        for (std::uint32_t i = 0u; i < config.texts.size(); ++i) {
            config.texts[i] = runtime.ReadTextProperty(
                combat + 0x1c4u + i * 16u);
            ++copied_texts;
        }
        for (std::uint32_t i = 0u; i < config.flags.size(); ++i)
            config.flags[i] = runtime.ReadByte(combat + 0x1f4u + i);
        config.words[3] = monster_id;
        runtime.ApplyConfiguration(config);
    } catch (...) {
        while (copied_texts != 0u) {
            --copied_texts;
            (void)runtime.ReleaseText(config.texts[copied_texts], copied_texts);
        }
        throw;
    }

    std::uint32_t result = 0u;
    while (copied_texts != 0u) {
        --copied_texts;
        result = runtime.ReleaseText(config.texts[copied_texts], copied_texts);
    }
    return result;
}

}  // namespace pal4
