#include "pal4/script_system_ui.h"

namespace pal4 {

void ScriptSystemUi::Grant(
    std::uint32_t index, std::uint8_t enabled,
    IScriptSystemUiRuntime& runtime) {
    if (index < 8u)
        runtime.SetGameDataEntry(runtime.Game(), index + 8u, enabled);
}

void ScriptSystemUi::Open(
    std::uint32_t index, IScriptSystemUiRuntime& runtime) {
    if (index > 6u) return;
    runtime.InitializeStateAndUi(runtime.Game(), 2u, 0u);
    runtime.ProcessEvent(runtime.UiFrameManager() + 0x104u,
                         0x8a80b8u, index + 1u, 0u);
}

void ScriptSystemUi::GrantSmith(
    std::uint8_t first, std::uint8_t second,
    std::uint8_t third, std::uint8_t fourth,
    IScriptSystemUiRuntime& runtime) {
    runtime.SetGameDataEntry(runtime.Game(), 15u, first);
    runtime.SetGameDataEntry(runtime.Game(), 16u, second);
    runtime.SetGameDataEntry(runtime.Game(), 17u, third);
    runtime.SetGameDataEntry(runtime.Game(), 18u, fourth);
}

void ScriptSystemUi::GrantMagic(
    std::uint8_t first, std::uint8_t second,
    IScriptSystemUiRuntime& runtime) {
    runtime.SetGameDataEntry(runtime.Game(), 20u, first);
    runtime.SetGameDataEntry(runtime.Game(), 19u, second);
}

void ScriptSystemUi::SetArenaSkillEnabled(
    std::uint8_t enabled, IScriptSystemUiRuntime& runtime) {
    runtime.SetFrameFlag(runtime.UiFrameManager(), enabled);
}

}  // namespace pal4
