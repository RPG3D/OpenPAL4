#include "pal4/ui_frame_manager_singleton.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct State final : pal4::IUiFrameManagerStateEventRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> events;
    std::uint32_t game_reads = 0;
    std::uint32_t state = 0;

    std::uint32_t GameInstance() override {
        ++game_reads;
        events.emplace_back("game");
        return game_reads == 1u ? 0x5078u : 0x6079u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        events.emplace_back("read");
        return words[address];
    }
    std::uint32_t QueryGameState(std::uint32_t controller) override {
        Check(controller == 0x8000u, "second controller is reread");
        events.emplace_back("state");
        return state;
    }
    std::uint8_t ProcessEvent(std::uint32_t tree,
                              std::uint32_t key,
                              std::uint32_t type,
                              std::uint32_t zero) override {
        Check(tree == 0x5104u && key == 0x8a80a4u &&
                  type == 0xabu && zero == 0u,
              "state event tree and payload");
        events.emplace_back("event");
        return 0x5au;
    }
};

struct Visibility final : pal4::IUiFrameManagerVisibilityRuntime {
    std::uint32_t frame = 0u;
    bool visible = false;
    std::vector<std::string> events;

    std::uint32_t FindFrame(std::uint32_t manager,
                            const char* name) override {
        Check(manager == 0x5000u && std::string(name) == "portrait",
              "visibility lookup uses portrait");
        events.emplace_back("find");
        return frame;
    }
    bool IsVisible(std::uint32_t receiver) override {
        Check(receiver == 0x7000u, "visibility receiver");
        events.emplace_back("visible");
        return visible;
    }
};

void CheckStateBranches() {
    State no_controller;
    Check(pal4::UiFrameManagerSingleton::ProcessStateEvent(
              0x5000u, 0x1abu, no_controller) == 0x78u &&
              no_controller.events ==
                  std::vector<std::string>{"game", "read"},
          "missing controller returns first game pointer low byte");

    State other_state;
    other_state.words[0x5204u] = 0x7000u;
    other_state.words[0x6205u] = 0x8000u;
    other_state.state = 0x123456feu;
    Check(pal4::UiFrameManagerSingleton::ProcessStateEvent(
              0x5000u, 0x1abu, other_state) == 0xfeu &&
              other_state.events == std::vector<std::string>{
                  "game", "read", "game", "read", "state"},
          "non-one state returns its low byte and skips event");

    State active;
    active.words[0x5204u] = 0x7000u;
    active.words[0x6205u] = 0x8000u;
    active.state = 1u;
    Check(pal4::UiFrameManagerSingleton::ProcessStateEvent(
              0x5000u, 0x1abu, active) == 0x5au &&
              active.events == std::vector<std::string>{
                  "game", "read", "game", "read", "state", "event"},
          "state exactly one dispatches masked event and forwards result");
}

void CheckVisibilityBranches() {
    Visibility missing;
    Check(pal4::UiFrameManagerSingleton::IsPortraitTimerVisible(
              0x5000u, missing) &&
              missing.events == std::vector<std::string>{"find"},
          "missing portrait frame is treated as visible");

    Visibility hidden;
    hidden.frame = 0x7000u;
    Check(!pal4::UiFrameManagerSingleton::IsPortraitTimerVisible(
              0x5000u, hidden) &&
              hidden.events ==
                  std::vector<std::string>{"find", "visible"},
          "present hidden frame returns false");

    Visibility visible;
    visible.frame = 0x7000u;
    visible.visible = true;
    Check(pal4::UiFrameManagerSingleton::IsPortraitTimerVisible(
              0x5000u, visible) &&
              visible.events ==
                  std::vector<std::string>{"find", "visible"},
          "present visible frame returns true");
}

}  // namespace

int main() {
    try {
        CheckStateBranches();
        CheckVisibilityBranches();
        std::cout << "ui_frame_manager_state_events_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
