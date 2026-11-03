#include "pal4/script_forbidden_skill.h"

namespace pal4 {

std::uint8_t ScriptForbiddenSkill::Set(
    const std::int32_t player_index, const std::int32_t skill,
    const std::uint32_t enabled, IScriptForbiddenSkillRuntime& runtime) {
    const auto player = runtime.FindPlayer(runtime.PlayerManager(),
                                           player_index);
    return player == 0u ? 0u :
        runtime.SetPresent(player + 0x938u, skill, enabled);
}

}  // namespace pal4
