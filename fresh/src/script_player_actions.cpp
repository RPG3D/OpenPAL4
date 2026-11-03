#include "pal4/script_player_actions.h"

namespace pal4 {

std::uint32_t ScriptPlayerActions::SetLeader(
    std::int32_t index, IScriptPlayerActionsRuntime& runtime) {
    return runtime.UpdatePlayerStates(runtime.PlayerManager(), index);
}

std::uint32_t ScriptPlayerActions::SetVisible(
    std::int32_t index, bool visible, IScriptPlayerActionsRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
    if (player == 0u) return 0u;
    const auto arena = runtime.ArenaManager();
    if (!visible) return runtime.ScriptOnRemove(arena, player);
    (void)runtime.ScriptOnRemove(arena, player);
    runtime.SetStateFlag(player, 1u);
    return runtime.ScriptOnAdd(runtime.ArenaManager(), player);
}

std::uint32_t ScriptPlayerActions::AttachCollision(
    std::int32_t index, IScriptPlayerActionsRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
    if (player == 0u) return 0u;
    const auto property = runtime.ReadWord(player + 124u * 4u);
    if (property == 0u) return property;
    const auto clump = runtime.ReadWord(player + 5u * 4u);
    const auto adapter = runtime.ReadWord(runtime.MovementAdapter());
    const auto result = runtime.SetClumpProperties(
        adapter, clump, 1u, 1u, 0u, 0u);
    runtime.WriteWord(player + 708u * 4u, 1u);
    return result;
}

std::uint32_t ScriptPlayerActions::DetachCollision(
    std::int32_t index, IScriptPlayerActionsRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
    if (player == 0u) return 0u;
    const auto attached = runtime.ReadWord(player + 708u * 4u);
    if (attached == 0u) return attached;
    const auto clump = runtime.ReadWord(player + 5u * 4u);
    const auto adapter = runtime.ReadWord(runtime.MovementAdapter());
    const auto result = runtime.SetClumpRenderCallback(adapter, clump);
    runtime.WriteWord(player + 708u * 4u, 0u);
    return result;
}

}  // namespace pal4
