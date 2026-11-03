#include "pal4/script_shadow_control.h"

namespace pal4 {

std::uint32_t ScriptShadowControl::Enable(
    const std::uint8_t enabled, IScriptShadowRuntime& runtime) {
    const auto flag = enabled == 0u ? 0u : 1u;
    return runtime.ForAllPlayers(
        runtime.PlayerManager(),
        [&runtime](const std::uint32_t player, const std::uint32_t value) {
            return ApplyToPlayer(player, value, runtime);
        }, flag);
}

std::uint32_t ScriptShadowControl::ApplyToPlayer(
    const std::uint32_t player, const std::uint32_t enabled,
    IScriptShadowRuntime& runtime) {
    if (player == 0u) return player;
    const auto arena_state = runtime.ReadWord(runtime.ArenaManager() + 0x10u);
    runtime.SetPlayerShadow(player, enabled, arena_state);
    return player;
}

}  // namespace pal4
