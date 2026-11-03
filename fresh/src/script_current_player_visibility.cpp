#include "pal4/script_current_player_visibility.h"

#include "pal4/player_manager_object.h"

namespace pal4 {

std::uint32_t ScriptCurrentPlayerVisibility::Set(
    std::uint8_t visible, IScriptCurrentPlayerVisibilityRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto index = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + index * PlayerManagerObject::kElementBytes;
    if (player == 0u) return manager;

    const auto arena = runtime.ArenaManager();
    if (visible == 0u) return runtime.ScriptOnRemove(arena, player);

    (void)runtime.ScriptOnRemove(arena, player);
    runtime.SetStateFlag(player, 1u);
    return runtime.ScriptOnAdd(runtime.ArenaManager(), player);
}

}  // namespace pal4
