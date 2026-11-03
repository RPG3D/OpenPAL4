#include "pal4/script_arena_skill_state.h"

namespace pal4 {

std::uint32_t ScriptArenaSkillState::HideSkillObject(
    IScriptArenaSkillStateRuntime& runtime) {
    return runtime.CleanupPlayerData(runtime.ArenaSkillManager());
}

std::uint32_t ScriptArenaSkillState::ShowSignpost(
    IScriptArenaSkillStateRuntime& runtime) {
    return runtime.UpdatePlayerData(runtime.ArenaSkillManager());
}

}  // namespace pal4
