#include "pal4/script_combat_config.h"

namespace pal4 {

std::uint32_t ScriptCombatConfig::Configure(
    std::uint32_t enabled_byte, std::uint32_t slot,
    std::uint32_t value, std::uint32_t type,
    IScriptCombatConfigRuntime& runtime) {
    const auto combat = runtime.CombatSystem();
    CombatConfigSnapshot config;
    for (std::uint32_t i = 0; i < config.words.size(); ++i)
        config.words[i] = runtime.ReadWord(combat + 432u + i * 4u);

    std::uint32_t copied_texts = 0u;
    try {
        for (std::uint32_t i = 0; i < config.texts.size(); ++i) {
            config.texts[i] = runtime.ReadTextProperty(combat + 452u + i * 16u);
            ++copied_texts;
        }
        config.words[0] = slot;
        config.words[1] = value;
        if (type <= 2u) config.words[4] = type;
        config.flags = {static_cast<std::uint8_t>(enabled_byte), 1u, 1u};
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
