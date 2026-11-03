#include "pal4/combat_console_hint_events.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatConsoleHintRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t shown_text = 0u;
    std::uint32_t shown_window = 0u;
    std::uint32_t lookup_key = 0u;

    Runtime() {
        words[0x3000u + 0x324u] = 2u;
        words[0x5000u + 4u] = 0x6000u;
    }

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t DialogData(std::uint32_t argument) override {
        Check(argument == 0x2000u, "dialog argument");
        calls.emplace_back("dialog");
        return 0x3000u;
    }
    std::uint32_t UiManager() override {
        calls.emplace_back("ui-manager");
        return 0x4000u;
    }
    std::uint32_t HintWindow(std::uint32_t manager) override {
        Check(manager == 0x4000u, "hint manager");
        calls.emplace_back("hint-window");
        return 0x7000u;
    }
    std::uint32_t CombatHintWindow2(std::uint32_t manager) override {
        Check(manager == 0x4000u, "combat hint manager");
        calls.emplace_back("combat-window");
        return 0x8000u;
    }
    void CloseHint(std::uint32_t window) override {
        Check(window == 0x7000u, "closed hint window");
        calls.emplace_back("close-hint");
    }
    void CleanupCombatHint(std::uint32_t window) override {
        Check(window == 0x8000u, "cleaned combat window");
        calls.emplace_back("cleanup-combat");
    }
    std::uint32_t GameDbInstance() override {
        calls.emplace_back("game-db");
        return 0x9000u;
    }
    std::uint32_t FindGameString(std::uint32_t manager,
                                 std::uint32_t key) override {
        Check(manager == 0x9000u + 52u, "GameString manager offset");
        calls.emplace_back("find-string");
        lookup_key = key;
        return 0x5000u;
    }
    std::uint32_t EmptyText() override {
        calls.emplace_back("empty-text");
        return 0xa000u;
    }
    std::uint32_t ShowHint(
        std::uint32_t window, std::uint32_t text,
        const std::array<std::uint32_t, 2>& position,
        std::uint32_t duration, std::uint32_t first,
        std::uint32_t second) override {
        Check(position == std::array<std::uint32_t, 2>{0x3f000000u,
                                                        0x3e19999au} &&
                  duration == 0x40000000u && first == 0u && second == 0u,
              "hint display arguments");
        calls.emplace_back("show-hint");
        shown_window = window;
        shown_text = text;
        return 0xb000u;
    }
    std::uint32_t ShowCombatHint(
        std::uint32_t window, std::uint32_t text,
        const std::array<std::uint32_t, 2>& position,
        std::uint32_t duration, std::uint32_t first,
        std::uint32_t second) override {
        Check(position == std::array<std::uint32_t, 2>{0x3f000000u,
                                                        0x3e19999au} &&
                  duration == 0x40000000u && first == 0u && second == 0u,
              "combat display arguments");
        calls.emplace_back("show-combat");
        shown_window = window;
        shown_text = text;
        return 0xc000u;
    }
};

std::size_t Position(const Runtime& runtime, std::string_view call) {
    const auto found = std::find(runtime.calls.begin(), runtime.calls.end(),
                                 call);
    Check(found != runtime.calls.end(), "expected call missing");
    return static_cast<std::size_t>(found - runtime.calls.begin());
}

void DefaultHint() {
    Runtime runtime;
    Check(pal4::CombatConsoleHintEvents::ShowDefault(0xdeadbeefu,
                                                    runtime) == 0xc000u,
          "default return value");
    Check(runtime.lookup_key == 0x139u && runtime.shown_text == 0x6000u &&
              runtime.shown_window == 0x8000u &&
              Position(runtime, "close-hint") <
                  Position(runtime, "game-db") &&
              Position(runtime, "find-string") <
                  Position(runtime, "combat-window") &&
              Position(runtime, "combat-window") <
                  Position(runtime, "show-combat"),
          "default callback order and key");
    Check(std::find(runtime.calls.begin(), runtime.calls.end(), "dialog") ==
              runtime.calls.end(), "default callback ignores argument");
}

void ConditionalHints() {
    Runtime first;
    Check(pal4::CombatConsoleHintEvents::ShowFleeingMonsterHint(
              0x2000u, first) == 0xb000u &&
              first.lookup_key == 0x103u && first.shown_text == 0x6000u &&
              first.shown_window == 0x7000u &&
              Position(first, "dialog") <
                  Position(first, "combat-window") &&
              Position(first, "cleanup-combat") <
                  Position(first, "game-db") &&
              Position(first, "find-string") <
                  Position(first, "hint-window"),
          "key 103 callback");

    Runtime second;
    second.words[0x5004u] = 0u;
    Check(pal4::CombatConsoleHintEvents::ShowFleeFailedMonsterHint(
              0x2000u, second) == 0xb000u &&
              second.lookup_key == 0x10du &&
              second.shown_text == 0xa000u &&
              Position(second, "empty-text") <
                  Position(second, "hint-window"),
          "key 10d missing-text fallback");
}

void StateGate() {
    for (const std::uint32_t state : {0u, 1u, 3u, 0xffffffffu}) {
        Runtime runtime;
        runtime.words[0x3000u + 0x324u] = state;
        Check(pal4::CombatConsoleHintEvents::ShowFleeingMonsterHint(
                  0x2000u, runtime) == 0x3000u,
              "state gate returns dialog object");
        Check(runtime.calls.size() == 2u &&
                  runtime.calls[0] == "dialog" &&
                  runtime.calls[1] ==
                      "read:" + std::to_string(0x3000u + 0x324u),
              "state gate does not touch UI or DB");
    }
}

void EventTextOffsets() {
    using Handler = std::uint32_t (*)(
        std::uint32_t, pal4::ICombatConsoleHintRuntime&);
    const std::array<std::pair<Handler, std::uint32_t>, 3> cases{{
        {pal4::CombatConsoleHintEvents::ShowEventTextAt16, 16u},
        {pal4::CombatConsoleHintEvents::ShowEventTextAt20, 20u},
        {pal4::CombatConsoleHintEvents::ShowEventTextAt12, 12u},
    }};
    for (const auto& [handler, offset] : cases) {
        Runtime runtime;
        runtime.words[0x2000u + 8u] = 0xd000u;
        runtime.words[0xd000u + offset] = 0xe000u;
        Check(handler(0x2000u, runtime) == 0xc000u &&
                  runtime.shown_text == 0xe000u &&
                  runtime.shown_window == 0x8000u &&
                  Position(runtime, "close-hint") <
                      Position(runtime, "read:8200") &&
                  Position(runtime, "combat-window") <
                      Position(runtime, "show-combat") &&
                  std::find(runtime.calls.begin(), runtime.calls.end(),
                            "game-db") == runtime.calls.end(),
              "event payload text offset and order");

        Runtime empty;
        empty.words[0x2000u + 8u] = 0xd000u;
        Check(handler(0x2000u, empty) == 0xc000u &&
                  empty.shown_text == 0xa000u &&
                  Position(empty, "empty-text") <
                      Position(empty, "combat-window"),
              "event text fallback");
    }
}

void Key10cHint() {
    Runtime runtime;
    Check(pal4::CombatConsoleHintEvents::ShowKey10c(
              0xdeadbeefu, runtime) == 0xb000u &&
              runtime.lookup_key == 0x10cu &&
              runtime.shown_text == 0x6000u &&
              Position(runtime, "cleanup-combat") <
                  Position(runtime, "game-db") &&
              Position(runtime, "find-string") <
                  Position(runtime, "hint-window"),
          "key 10c hint order and return");
}

}  // namespace

int main() {
    try {
        DefaultHint();
        ConditionalHints();
        StateGate();
        EventTextOffsets();
        Key10cHint();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
