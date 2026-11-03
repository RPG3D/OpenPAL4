#include "pal4/pal_game_iv_state_transition.h"

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

struct Runtime final : pal4::IPalGameIVStateRuntime {
    std::vector<std::string> events;
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::uint32_t entry = 0u;
    std::uint32_t old_id = 3u;
    bool enter_result = true;
    std::uint32_t atexit_calls = 0u;
    std::uint32_t state_id_calls = 0u;
    std::uint32_t clock_calls = 0u;
    std::uint32_t enter_previous = 0u;
    std::uint32_t enter_argument = 0u;
    std::string logged_previous;
    std::string logged_next;

    void RegisterAtExit(std::uint32_t callback) override {
        Check(callback == 0x5b67a0u, "atexit callback");
        ++atexit_calls;
        events.push_back("atexit");
    }
    std::uint32_t FindEntry(std::uint32_t game,
                             std::uint32_t id) override {
        Check(game == 0x1000u && id == 2u, "entry lookup");
        events.push_back("find");
        return entry;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x118cu, "current state address");
        events.push_back("read-current");
        return words[address];
    }
    void WriteWord(std::uint32_t address,
                   std::uint32_t value) override {
        Check(address == 0x118cu && value == entry,
              "current state update");
        events.push_back("write-current");
        words[address] = value;
    }
    void Leave(std::uint32_t state, std::uint32_t next_id) override {
        Check(state == 0x3000u && next_id == 2u, "leave arguments");
        events.push_back("leave");
    }
    std::uint32_t StateId(std::uint32_t state) override {
        Check(state == 0x3000u, "state id object");
        ++state_id_calls;
        events.push_back("state-id");
        return old_id;
    }
    std::uint32_t UiFrameManager() override {
        events.push_back("ui-manager");
        return 0x4000u;
    }
    void SetCursor(std::uint32_t cursor_slot,
                   std::string_view resource) override {
        Check(cursor_slot == 0x4114u && resource == "default.cur",
              "cursor arguments");
        events.push_back("cursor");
    }
    void LogExchange(std::string_view previous,
                     std::string_view next,
                     std::uint32_t source_line) override {
        Check(source_line == 0x1a1u, "exchange log line");
        logged_previous = previous;
        logged_next = next;
        events.push_back("log");
    }
    bool Enter(std::uint32_t state, std::uint32_t previous_id,
               std::uint32_t argument) override {
        Check(state == entry, "enter target");
        enter_previous = previous_id;
        enter_argument = argument;
        events.push_back("enter");
        return enter_result;
    }
    std::uint32_t TimeGetTime() override {
        ++clock_calls;
        events.push_back("clock");
        return 0x12345678u;
    }
    void WriteGameStateWord(std::uint32_t offset,
                            std::uint32_t value) override {
        Check((offset == 0x574u || offset == 0x56cu) &&
                  value == 0x12345678u, "timestamp write");
        events.push_back(offset == 0x574u ? "time-one" : "time-two");
        words[0x5000u + offset] = value;
    }
};

void FirstInitialization() {
    pal4::PalGameIVStateLabels labels;
    labels.guard = 0xaau;
    Runtime runtime;
    Check(!pal4::PalGameIVStateTransition::Initialize(
              0x1000u, 2u, 77u, labels, runtime), "missing entry result");
    Check(labels.guard == 0xabu && runtime.atexit_calls == 1u &&
              runtime.events == std::vector<std::string>({"atexit", "find"}),
          "one-time label initialization");
    const std::array<std::string_view, 13> names = {
        "PALIV_LOGO", "PALIV_SCENE", "PALIV_SYSTEM", "PALIV_COMBAT",
        "PALIV_TRADE", "PALIV_FOUNDRY", "PALIV_LOADING",
        "PALIV_GAME_OVER", "PALIV_MINIGAME", "PALIV_GAME_SAVE",
        "PALIV_MODAL_FRAME", "PALIV_WORLD_MAP", "PALIV_BINK",
    };
    for (std::size_t index = 0; index < names.size(); ++index)
        Check(labels.names[index] == names[index] &&
                  labels.markers[index] == 2u &&
                  labels.error_flags[index] == 0u, "state label data");

    runtime.entry = 0x2000u;
    runtime.events.clear();
    Check(pal4::PalGameIVStateTransition::Initialize(
              0x1000u, 2u, 99u, labels, runtime), "first transition");
    Check(runtime.events == std::vector<std::string>({
              "find", "read-current", "ui-manager", "cursor", "enter",
              "write-current", "clock", "time-one", "time-two"}),
          "first transition order");
    Check(runtime.enter_previous == 13u && runtime.enter_argument == 99u &&
              runtime.words[0x118cu] == 0x2000u &&
              runtime.clock_calls == 1u && runtime.atexit_calls == 1u,
          "first transition state");
    pal4::PalGameIVStateTransition::Cleanup(labels);
    for (const auto& name : labels.names)
        Check(name.empty(), "cleanup label");
    Check(labels.guard == 0xabu, "cleanup keeps guard");
}

void PreviousStateAndFailure() {
    pal4::PalGameIVStateLabels labels;
    Runtime runtime;
    runtime.entry = 0x2000u;
    runtime.words[0x118cu] = 0x3000u;
    runtime.enter_result = false;
    Check(!pal4::PalGameIVStateTransition::Initialize(
              0x1000u, 2u, 1u, labels, runtime), "failed enter");
    Check(runtime.events == std::vector<std::string>({
              "atexit", "find", "read-current", "leave", "state-id",
              "ui-manager", "cursor", "state-id", "log", "enter"}),
          "previous-state order");
    Check(runtime.state_id_calls == 2u &&
              runtime.logged_previous == "PALIV_COMBAT" &&
              runtime.logged_next == "PALIV_SYSTEM" &&
              runtime.enter_previous == 3u &&
              runtime.words[0x118cu] == 0x3000u && runtime.clock_calls == 0u,
          "failed transition leaves active state unchanged");
    runtime.enter_result = true;
    runtime.events.clear();
    Check(pal4::PalGameIVStateTransition::Initialize(
              0x1000u, 2u, 4u, labels, runtime), "successful enter");
    Check(runtime.words[0x118cu] == 0x2000u &&
              runtime.words[0x5000u + 0x574u] == 0x12345678u &&
              runtime.words[0x5000u + 0x56cu] == 0x12345678u &&
              runtime.enter_argument == 4u, "successful transition state");
}

}  // namespace

int main() {
    try {
        FirstInitialization();
        PreviousStateAndFailure();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
