#include "pal4/script_combat_config.h"

namespace pal4 {
std::uint8_t ScriptCombatTextThunks::SetBgm(
    std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime) {
    return CombatSystemPropertyCopy::AssignFirstText(source, runtime);
}

std::uint8_t ScriptCombatTextThunks::SetCamera(
    std::uint32_t source, ICombatSystemPropertyCopyRuntime& runtime) {
    return CombatSystemPropertyCopy::AssignSecondText(source, runtime);
}

}  // namespace pal4
