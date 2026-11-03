#include "pal4/combat_console_hint_events.h"

namespace pal4 {

namespace {
constexpr std::array<std::uint32_t, 2> kPosition{
    0x3f000000u, 0x3e19999au};
constexpr std::uint32_t kDuration = 0x40000000u;
}

std::uint32_t CombatConsoleHintEvents::GameText(
    const std::uint32_t string_key, ICombatConsoleHintRuntime& runtime) {
    const auto record = GameStringReference(string_key).FindInGameDB(runtime);
    const auto text = runtime.ReadWord(record + 4u);
    return text == 0u ? runtime.EmptyText() : text;
}

std::uint32_t CombatConsoleHintEvents::ShowDefault(
    const std::uint32_t argument, ICombatConsoleHintRuntime& runtime) {
    (void)argument;
    runtime.CloseHint(runtime.HintWindow(runtime.UiManager()));
    const auto text = GameText(0x139u, runtime);
    return runtime.ShowCombatHint(
        runtime.CombatHintWindow2(runtime.UiManager()), text,
        kPosition, kDuration, 0u, 0u);
}

std::uint32_t CombatConsoleHintEvents::ShowConditional(
    const std::uint32_t argument, const std::uint32_t string_key,
    ICombatConsoleHintRuntime& runtime) {
    const auto dialog = runtime.DialogData(argument);
    if (!ShouldShowConditional(dialog,runtime)) return dialog;

    runtime.CleanupCombatHint(
        runtime.CombatHintWindow2(runtime.UiManager()));
    const auto text = GameText(string_key, runtime);
    return runtime.ShowHint(runtime.HintWindow(runtime.UiManager()), text,
                            kPosition, kDuration, 0u, 0u);
}

std::uint32_t CombatConsoleHintEvents::ShowFleeingMonsterHint(
    const std::uint32_t argument, ICombatConsoleHintRuntime& runtime) {
    return ShowConditional(argument, 0x103u, runtime);
}

std::uint32_t CombatConsoleHintEvents::ShowFleeFailedMonsterHint(
    const std::uint32_t argument, ICombatConsoleHintRuntime& runtime) {
    return ShowConditional(argument, 0x10du, runtime);
}

std::uint32_t CombatConsoleHintEvents::ShowEventText(
    const std::uint32_t event, const std::uint32_t text_offset,
    ICombatConsoleHintRuntime& runtime) {
    runtime.CloseHint(runtime.HintWindow(runtime.UiManager()));
    const auto payload = runtime.ReadWord(event + 8u);
    const auto source = runtime.ReadWord(payload + text_offset);
    const auto text = source == 0u ? runtime.EmptyText() : source;
    return runtime.ShowCombatHint(
        runtime.CombatHintWindow2(runtime.UiManager()), text,
        kPosition, kDuration, 0u, 0u);
}

std::uint32_t CombatConsoleHintEvents::ShowEventTextAt16(
    const std::uint32_t event, ICombatConsoleHintRuntime& runtime) {
    return ShowEventText(event, 16u, runtime);
}

std::uint32_t CombatConsoleHintEvents::ShowEventTextAt20(
    const std::uint32_t event, ICombatConsoleHintRuntime& runtime) {
    return ShowEventText(event, 20u, runtime);
}

std::uint32_t CombatConsoleHintEvents::ShowEventTextAt12(
    const std::uint32_t event, ICombatConsoleHintRuntime& runtime) {
    return ShowEventText(event, 12u, runtime);
}

std::uint32_t CombatConsoleHintEvents::ShowKey10c(
    const std::uint32_t argument, ICombatConsoleHintRuntime& runtime) {
    (void)argument;
    runtime.CleanupCombatHint(
        runtime.CombatHintWindow2(runtime.UiManager()));
    const auto text = GameText(0x10cu, runtime);
    return runtime.ShowHint(runtime.HintWindow(runtime.UiManager()), text,
                            kPosition, kDuration, 0u, 0u);
}

}  // namespace pal4
