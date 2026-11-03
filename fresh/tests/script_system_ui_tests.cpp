#include "pal4/script_system_ui.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Runtime final : pal4::IScriptSystemUiRuntime {
    std::vector<std::string> calls;
    std::uint32_t expected_index = 0u;
    std::uint8_t expected_value = 0u;

    std::uint32_t Game() override {
        calls.emplace_back("game");
        return 0x1000u;
    }
    void SetGameDataEntry(std::uint32_t game, std::uint32_t entry,
                          std::uint8_t value) override {
        Check(game == 0x1000u, "game data instance");
        calls.emplace_back("entry:" + std::to_string(entry) + ":" +
                           std::to_string(value));
    }
    void InitializeStateAndUi(std::uint32_t game,
                              std::uint32_t state,
                              std::uint32_t flags) override {
        Check(game == 0x1000u && state == 2u && flags == 0u,
              "UI state transition operands");
        calls.emplace_back("initialize UI");
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("frames");
        return 0x2000u;
    }
    void ProcessEvent(std::uint32_t event_tree,
                      std::uint32_t event_descriptor,
                      std::uint32_t menu_index,
                      std::uint32_t reserved) override {
        Check(event_tree == 0x2104u && event_descriptor == 0x8a80b8u &&
                  menu_index == expected_index + 1u && reserved == 0u,
              "system UI event arguments");
        calls.emplace_back("event");
    }
    void SetFrameFlag(std::uint32_t frames,
                      std::uint8_t enabled) override {
        Check(frames == 0x2000u && enabled == expected_value,
              "arena skill frame flag operands");
        calls.emplace_back("frame flag");
    }
};

void GrantBranches() {
    for (std::uint32_t index = 0u; index < 8u; ++index) {
        Runtime runtime;
        runtime.expected_index = index;
        runtime.expected_value = static_cast<std::uint8_t>(index * 31u);
        pal4::ScriptSystemUi::Grant(index, runtime.expected_value, runtime);
        Check(runtime.calls == std::vector<std::string>{
                  "game", "entry:" + std::to_string(index + 8u) + ":" +
                              std::to_string(runtime.expected_value)},
              "all eight grantable UI entries");
    }
    for (auto index : {8u, 0xffffffffu}) {
        Runtime runtime;
        pal4::ScriptSystemUi::Grant(index, 255u, runtime);
        Check(runtime.calls.empty(), "out-of-range grant skips game access");
    }
}

void FixedGrantBranches() {
    Runtime smith;
    pal4::ScriptSystemUi::GrantSmith(1u, 0u, 255u, 2u, smith);
    Check(smith.calls == std::vector<std::string>{
              "game", "entry:15:1", "game", "entry:16:0",
              "game", "entry:17:255", "game", "entry:18:2"},
          "smith grants each acquire game and retain original byte");
    Runtime magic;
    pal4::ScriptSystemUi::GrantMagic(0u, 255u, magic);
    Check(magic.calls == std::vector<std::string>{
              "game", "entry:20:0", "game", "entry:19:255"},
          "magic grants use 20 then 19 with separate game accesses");
}

void ArenaSkillFlag() {
    for (auto value : {0u, 1u, 255u}) {
        Runtime runtime;
        runtime.expected_value = static_cast<std::uint8_t>(value);
        pal4::ScriptSystemUi::SetArenaSkillEnabled(
            runtime.expected_value, runtime);
        Check(runtime.calls == std::vector<std::string>{
                  "frames", "frame flag"},
              "arena skill flag uses frame manager and low byte");
    }
}

void OpenBranches() {
    for (std::uint32_t index = 0u; index <= 6u; ++index) {
        Runtime runtime;
        runtime.expected_index = index;
        pal4::ScriptSystemUi::Open(index, runtime);
        Check(runtime.calls == std::vector<std::string>{
                  "game", "initialize UI", "frames", "event"},
              "system UI state and event order");
    }
    for (auto index : {7u, 0xffffffffu}) {
        Runtime runtime;
        pal4::ScriptSystemUi::Open(index, runtime);
        Check(runtime.calls.empty(), "invalid menu index skips UI work");
    }
}

}  // namespace

int main() {
    try {
        GrantBranches();
        OpenBranches();
        FixedGrantBranches();
        ArenaSkillFlag();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
