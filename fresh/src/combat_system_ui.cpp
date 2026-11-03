#include "pal4/combat_system_ui.h"

namespace pal4 {

std::uint32_t CombatSystemUi::EndSequence(
    const std::uint32_t object, const std::uint32_t result,
    ICombatSystemUiRuntime& runtime) {
    static_cast<void>(CloseEndingWrapper(object, runtime));
    runtime.SyncPlayers();
    runtime.WriteWord(object + 508u, 7u);
    runtime.WriteWord(object + 504u, result);
    static_cast<void>(CloseWrapper(object, runtime));
    runtime.EndCombat(object);
    return runtime.DispatchScriptEvent(
        "close_system", runtime.ReadWord(object + 504u));
}

std::uint32_t CombatSystemUi::ShowWrapper(
    const std::uint32_t object, ICombatSystemUiRuntime& runtime) {
    return Show(object, runtime);
}

std::uint32_t CombatSystemUi::Show(
    const std::uint32_t object, ICombatSystemUiRuntime& runtime) {
    if (runtime.ReadByte(object + 516u) != 0u) {
        runtime.ProcessUiEvent("show_quick_combat_result", 1u, 0u);
        runtime.ProcessUiEvent("show_quick_combat_result", 0u, 0u);
        runtime.ProcessUiEvent("show_quick_combat", 1u, 0u);
        return 1u;
    }
    runtime.ProcessUiEvent("show_combat_ending", 1u, 0u);
    runtime.ProcessUiEvent("show_combat_ending", 0u, 0u);
    runtime.ProcessUiEvent("show_combat_main_ui", 1u, 0u);
    const auto frame = runtime.FindFrame("CombatMainWindow");
    if (frame != 0u) {
        const auto parameter = runtime.ReadWord(frame + 36u);
        runtime.ProcessUiEvent("show_combat_action_console", 1u,
                               parameter);
        runtime.ProcessUiEvent("show_combat_role_state", 1u,
                               parameter);
    }
    return 1u;
}

std::uint32_t CombatSystemUi::CloseWrapper(
    const std::uint32_t object, ICombatSystemUiRuntime& runtime) {
    return Close(object, runtime);
}

std::uint32_t CombatSystemUi::Close(
    const std::uint32_t object, ICombatSystemUiRuntime& runtime) {
    if (runtime.ReadByte(object + 516u) != 0u) {
        runtime.ProcessUiEvent("show_quick_combat", 0u, 0u);
        runtime.ProcessUiEvent("show_quick_combat_result", 0u, 0u);
    } else {
        runtime.ProcessUiEvent("show_combat_main_ui", 0u, 0u);
        runtime.ProcessUiEvent("show_combat_action_console", 0u, 0u);
        runtime.ProcessUiEvent("show_combat_role_state", 0u, 0u);
        runtime.ProcessUiEvent("show_combat_ending", 0u, 0u);
    }
    runtime.CleanupHintWindow(runtime.CreateHintWindow(1u));
    runtime.CleanupHintWindow(runtime.CreateHintWindow(2u));
    return 1u;
}

std::uint32_t CombatSystemUi::ShowEndingWrapper(
    const std::uint32_t object, const std::uint32_t prize,
    ICombatSystemUiRuntime& runtime) {
    return ShowEnding(object, prize, runtime);
}

std::uint32_t CombatSystemUi::ShowEnding(
    const std::uint32_t object, const std::uint32_t prize,
    ICombatSystemUiRuntime& runtime) {
    runtime.WriteWord(object + 508u, 6u);
    runtime.UnsubscribeConsoleEvents(runtime.GetConsole());
    static_cast<void>(CloseWrapper(object, runtime));
    runtime.ProcessUiEvent("show_combat_ending", 1u, 0u);
    static_cast<void>(runtime.DispatchScriptEvent("combat_prize", prize));
    return 1u;
}

std::uint32_t CombatSystemUi::CloseEndingWrapper(
    const std::uint32_t object, ICombatSystemUiRuntime& runtime) {
    return CloseEnding(object, runtime);
}

std::uint32_t CombatSystemUi::CloseEnding(
    const std::uint32_t object, ICombatSystemUiRuntime& runtime) {
    static_cast<void>(object);
    runtime.ProcessUiEvent("show_combat_ending", 0u, 0u);
    return 1u;
}

}  // namespace pal4
