#include "pal4/script_action_hold_release.h"

namespace pal4 {

std::uint32_t ScriptActionHoldRelease::Player(
    std::int32_t index, IScriptActionHoldRuntime& runtime) {
    const auto player = runtime.PlayerByIndex(runtime.PlayerManager(), index);
    if (player == 0u) return 0u;
    runtime.ClearAnimationFlag(player + 0x170u, 0u);
    return runtime.UpdatePlayerEffects(player);
}

std::uint32_t ScriptActionHoldRelease::Npc(
    std::uint32_t payload, IScriptActionHoldRuntime& runtime) {
    auto name = runtime.ReadWord(payload + 4u);
    if (name == 0u) name = runtime.EmptyStringAddress();
    const auto npc = runtime.FindNpc(runtime.NpcManager(), name);
    if (npc == 0u) return 0u;
    runtime.ClearAnimationFlag(npc + 0x170u, 0u);
    return runtime.UpdateNpcEffects(npc);
}

}  // namespace pal4
