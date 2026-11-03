#include "pal4/script_player_status.h"

#include "pal4/player_manager_object.h"

namespace pal4 {

std::int32_t ScriptPlayerStatus::GetLeaderState(
    IScriptPlayerStatusRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * PlayerManagerObject::kElementBytes;
    if (player == 0u) return -1;
    const auto state = runtime.ReadWord(player + 0xa48u);
    return state <= 3u ? static_cast<std::int32_t>(state) : -1;
}

std::uint32_t ScriptPlayerStatus::SetLevel(
    std::int32_t player_id, std::int32_t level,
    IScriptPlayerStatusRuntime& runtime) {
    if (player_id < 0) return static_cast<std::uint32_t>(player_id);
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(),
                                              player_id);
    if (player == 0u || level <= 0 || level >= 100) return player;

    runtime.WriteLevel(player + 0x654u, 4u, level);
    runtime.RefreshPlayerState(player + 0x654u, level,
                               runtime.ReadWord(player + 0xa48u));
    const auto records = runtime.BuildMatchingRecords(
        player + 0x938u, level, runtime.ReadWord(player + 0xa48u));
    return runtime.FreeRecords(records);
}

}  // namespace pal4
