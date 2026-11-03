#include "pal4/script_entity_callbacks.h"

namespace pal4 {

std::uint8_t ScriptPlayerLockHandler::Lock(
    IScriptPlayerLockRuntime& runtime) {
    const auto manager = runtime.PlayerManager();
    const auto current = runtime.ReadWord(manager + 0x2c50u);
    const auto player = manager + current * 2836u;
    if (player != 0u) runtime.SetActionCode(player + 0x170u, "C01");
    runtime.ResetAndSync(runtime.ActorControl(), 0u);
    return runtime.ProcessUiEvents(runtime.UiFrameManager(), 0u);
}

void ScriptPlayerLockHandler::Unlock(IScriptPlayerLockRuntime& runtime) {
    runtime.ResetAndSync(runtime.ActorControl(), 1u);
    (void)runtime.ProcessUiEvents(runtime.UiFrameManager(), 1u);
    const auto object = runtime.ReadWord(runtime.ActorControl() + 4u);
    if (object != 0u)
        runtime.UpdateGrid(runtime.MinimapManager(), object + 0xacu,
                           object + 0xa0u);
}

std::uint32_t ScriptNpcVisibility::SetVisible(
    const char* name, bool visible, IScriptNpcVisibilityRuntime& runtime) {
    const auto npc = runtime.FindNpc(runtime.NpcManager(), name ? name : "");
    if (npc == 0u) return 0u;
    if (!visible) return runtime.LeaveNpcScript(runtime.ArenaManager(), npc);
    (void)runtime.LeaveNpcScript(runtime.ArenaManager(), npc);
    runtime.SetStateFlag(npc, 1u);
    return runtime.EnterNpcScript(runtime.ArenaManager(), npc);
}

}  // namespace pal4
