#include "pal4/combat_console_message_events.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

constexpr std::uint32_t kEvent = 0x1000u;

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleMessageRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::vector<std::uint32_t> format_arguments;
    std::string displayed_text;
    std::uint32_t game_string_key = 0u;
    std::uint32_t format_pointer = 0u;
    std::uint32_t sound_calls = 0u;
    bool fail_display = false;

    Runtime() {
        words[kEvent] = 0x21u;
        words[kEvent + 4u] = 3u;
        words[0x9100u + 20u] = 0x9200u;
        words[0x9300u + 4u] = 0x9400u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t DialogData(std::uint32_t) override { return 0u; }
    std::uint32_t GameDbInstance() override {
        calls.emplace_back("game-db");
        return 0x9000u;
    }
    std::uint32_t FindCombatRecord(std::uint32_t database,
                                   std::uint32_t key) override {
        Check(database == 0x9000u && key == 0x21u,
              "combat record lookup");
        calls.emplace_back("combat-record");
        return 0x9100u;
    }
    std::uint32_t FindGameString(std::uint32_t manager,
                                 std::uint32_t key) override {
        Check(manager == 0x9000u + 52u, "GameString manager");
        calls.emplace_back("game-string");
        game_string_key = key;
        return 0x9300u;
    }
    std::uint32_t UiManager() override {
        calls.emplace_back("ui-manager");
        return 0xa000u;
    }
    std::uint32_t HintWindow(std::uint32_t manager) override {
        Check(manager == 0xa000u, "hint UI manager");
        calls.emplace_back("hint-window");
        return 0xa100u;
    }
    std::uint32_t CombatHintWindow2(std::uint32_t manager) override {
        Check(manager == 0xa000u, "combat UI manager");
        calls.emplace_back("combat-window");
        return 0xa200u;
    }
    void CloseHint(std::uint32_t) override {}
    void CleanupCombatHint(std::uint32_t window) override {
        Check(window == 0xa200u, "combat hint cleanup");
        calls.emplace_back("cleanup-combat");
    }
    std::uint32_t EmptyText() override {
        calls.emplace_back("empty-text");
        return 0xb000u;
    }
    std::uint32_t ShowHint(
        std::uint32_t, std::uint32_t,
        const std::array<std::uint32_t, 2>&,
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    std::uint32_t ShowCombatHint(
        std::uint32_t, std::uint32_t,
        const std::array<std::uint32_t, 2>&,
        std::uint32_t, std::uint32_t,
        std::uint32_t) override { return 0u; }
    std::string FormatText(
        std::uint32_t format,
        std::span<const std::uint32_t> arguments) override {
        calls.emplace_back("format");
        format_pointer = format;
        format_arguments.assign(arguments.begin(), arguments.end());
        return "formatted";
    }
    void ShowFormattedHint(
        std::uint32_t window, std::string_view text,
        const std::array<std::uint32_t, 2>& position,
        std::uint32_t duration) override {
        Check(window == 0xa100u && text == "formatted" &&
                  position == std::array<std::uint32_t, 2>{
                      0x3f000000u, 0x3e19999au} &&
                  duration == 0x40000000u,
              "formatted hint presentation");
        calls.emplace_back("show-hint");
        displayed_text = text;
        if (fail_display) throw std::runtime_error("show failure");
    }
    std::uint32_t CombatSystem() override {
        calls.emplace_back("combat-system");
        return 0xc000u;
    }
    void PlaySoundEffect(std::uint32_t event,
                         std::string_view sound,
                         std::uint32_t mode) override {
        Check(event == kEvent && sound == "WA06.WAV" && mode == 1u,
              "sound effect arguments");
        calls.emplace_back("play-sound");
        ++sound_calls;
    }
    std::uint32_t CleanupFormattedText(std::string_view text) override {
        Check(text.empty() || text == "formatted",
              "temporary text cleanup contents");
        calls.emplace_back("cleanup-text");
        return 0xd000u;
    }
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void CombatRecordMessage() {
    Runtime runtime;
    Check(pal4::CombatConsoleMessageEvents::ShowCombatRecord(
              kEvent, runtime) == 0xd000u &&
              runtime.game_string_key == 0xfbu &&
              runtime.format_pointer == 0x9400u &&
              runtime.format_arguments ==
                  std::vector<std::uint32_t>{0x9200u, 3u} &&
              runtime.sound_calls == 1u &&
              Position(runtime, "combat-record") <
                  Position(runtime, "game-string") &&
              Position(runtime, "format") <
                  Position(runtime, "cleanup-combat") &&
              Position(runtime, "show-hint") <
                  Position(runtime, "combat-system") &&
              Position(runtime, "play-sound") <
                  Position(runtime, "cleanup-text"),
          "combat record formatting, UI and sound order");

    Runtime silent;
    silent.words[kEvent + 4u] = 0u;
    silent.words[0x9100u + 20u] = 0u;
    silent.words[0x9300u + 4u] = 0u;
    Check(pal4::CombatConsoleMessageEvents::ShowCombatRecord(
              kEvent, silent) == 0xd000u &&
              silent.format_pointer == 0xb000u &&
              silent.format_arguments ==
                  std::vector<std::uint32_t>{0xb000u, 0u} &&
              silent.sound_calls == 0u,
          "missing combat name and format use empty text");
}

void SignedValueMessage() {
    Runtime positive;
    positive.words[kEvent] = 13u;
    Check(pal4::CombatConsoleMessageEvents::ShowSignedValue(
              kEvent, positive) == 0xd000u &&
              positive.game_string_key == 0xc2u &&
              positive.format_arguments ==
                  std::vector<std::uint32_t>{13u} &&
              positive.sound_calls == 1u &&
              Position(positive, "cleanup-combat") <
                  Position(positive, "game-string") &&
              Position(positive, "show-hint") <
                  Position(positive, "play-sound"),
          "positive value format and sound");

    Runtime negative;
    negative.words[kEvent] = 0xfffffff6u;
    Check(pal4::CombatConsoleMessageEvents::ShowSignedValue(
              kEvent, negative) == 0xd000u &&
              negative.game_string_key == 0x10bu &&
              negative.format_arguments ==
                  std::vector<std::uint32_t>{10u} &&
              negative.sound_calls == 0u,
          "negative value format without sound");

    Runtime zero;
    zero.words[kEvent] = 0u;
    Check(pal4::CombatConsoleMessageEvents::ShowSignedValue(
              kEvent, zero) == 0xd000u &&
              zero.calls == std::vector<std::string>{
                  "read:4096", "cleanup-text"},
          "zero value only cleans empty temporary text");
}

void ExceptionCleansTemporaryText() {
    Runtime runtime;
    runtime.fail_display = true;
    bool caught = false;
    try {
        (void)pal4::CombatConsoleMessageEvents::ShowSignedValue(
            kEvent, runtime);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && Position(runtime, "show-hint") <
                        Position(runtime, "cleanup-text") &&
              runtime.sound_calls == 0u,
          "formatted text released during display exception");
}

}  // namespace

int main() {
    try {
        CombatRecordMessage();
        SignedValueMessage();
        ExceptionCleansTemporaryText();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
