#include "pal4/combat_console_message_events.h"

#include "pal4/game_string_reference.h"

#include <array>
#include <bit>
#include <cstdint>
#include <string>

namespace pal4 {

namespace {

constexpr std::array<std::uint32_t, 2> kPosition{
    0x3f000000u, 0x3e19999au};
constexpr std::uint32_t kDuration = 0x40000000u;

std::uint32_t TextOrEmpty(const std::uint32_t record,
                          ICombatConsoleMessageRuntime& runtime) {
    const auto text = runtime.ReadWord(record + 4u);
    return text == 0u ? runtime.EmptyText() : text;
}

std::uint32_t GameText(const std::uint32_t key,
                       ICombatConsoleMessageRuntime& runtime) {
    return TextOrEmpty(GameStringReference(key).FindInGameDB(runtime),
                       runtime);
}

void ClearCombatHint(ICombatConsoleMessageRuntime& runtime) {
    runtime.CleanupCombatHint(
        runtime.CombatHintWindow2(runtime.UiManager()));
}

void ShowFormattedHint(const std::string& text,
                       ICombatConsoleMessageRuntime& runtime) {
    runtime.ShowFormattedHint(runtime.HintWindow(runtime.UiManager()),
                              text, kPosition, kDuration);
}

}  // namespace

std::uint32_t CombatConsoleMessageEvents::ShowCombatRecord(
    const std::uint32_t event, ICombatConsoleMessageRuntime& runtime) {
    const auto database = runtime.GameDbInstance();
    const auto record = runtime.FindCombatRecord(
        database, runtime.ReadWord(event));
    const auto value = runtime.ReadWord(event + 4u);
    const auto source = runtime.ReadWord(record + 20u);
    const auto name = source == 0u ? runtime.EmptyText() : source;
    const auto format = GameText(0xfbu, runtime);
    const std::array arguments{name, value};
    const std::string text = runtime.FormatText(format, arguments);
    try {
        ClearCombatHint(runtime);
        ShowFormattedHint(text, runtime);
        if (std::bit_cast<std::int32_t>(value) > 0) {
            (void)runtime.CombatSystem();
            runtime.PlaySoundEffect(event, "WA06.WAV", 1u);
        }
    } catch (...) {
        (void)runtime.CleanupFormattedText(text);
        throw;
    }
    return runtime.CleanupFormattedText(text);
}

std::uint32_t CombatConsoleMessageEvents::ShowSignedValue(
    const std::uint32_t event, ICombatConsoleMessageRuntime& runtime) {
    const auto value = std::bit_cast<std::int32_t>(runtime.ReadWord(event));
    if (value == 0) return runtime.CleanupFormattedText({});

    ClearCombatHint(runtime);
    const auto format = GameText(value > 0 ? 0xc2u : 0x10bu, runtime);
    const std::array arguments{
        value > 0 ? std::bit_cast<std::uint32_t>(value) :
                    0u - std::bit_cast<std::uint32_t>(value)};
    const std::string text = runtime.FormatText(format, arguments);
    try {
        ShowFormattedHint(text, runtime);
        if (value > 0) {
            (void)runtime.CombatSystem();
            runtime.PlaySoundEffect(event, "WA06.WAV", 1u);
        }
    } catch (...) {
        (void)runtime.CleanupFormattedText(text);
        throw;
    }
    return runtime.CleanupFormattedText(text);
}

}  // namespace pal4
