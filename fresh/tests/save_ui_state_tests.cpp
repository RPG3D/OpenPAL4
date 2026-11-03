#include "pal4/save_ui_state.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ISaveUiRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t frame = 0u;
    std::uint8_t visible = 0u;
    Runtime() { words[0x5000u] = 0x1234u; }

    std::uint32_t ReadWord(std::uint32_t address) override {
        Check(address == 0x5000u, "render frame read");
        calls.emplace_back("read-frame"); return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x2000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x2000u && value == 1u, "save UI flag");
        calls.emplace_back("set-flag");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 0x3000u;
    }
    void DispatchEvent(std::uint32_t manager,
                       std::uint32_t descriptor,
                       std::uint32_t first,
                       std::uint32_t second) override {
        Check(manager == 0x2000u &&
              descriptor == pal4::SaveUiState::kShowSaveUiEvent &&
              first == 1u && second == 0u, "show-save UI event");
        calls.emplace_back("show-save-ui");
    }
    std::uint32_t ArenaManager() override {
        calls.emplace_back("arena"); return 0x4000u;
    }
    void ProcessWorldFrame(std::uint32_t arena,
                           std::uint32_t frame_value) override {
        Check(arena == 0x4000u && frame_value == 0x1234u,
              "world frame arguments");
        calls.emplace_back("process-world");
    }
    std::uint32_t RenderUI(std::uint32_t manager,
                            std::uint32_t rendered_frame) override {
        Check(manager == 0x2000u && rendered_frame == 0x5000u,
              "UI render arguments");
        calls.emplace_back("render-ui"); return 0x77u;
    }
    std::uint32_t FindFrame(std::uint32_t manager,
                             std::string_view name) override {
        Check(manager == 0x2000u && name == "SaveWindow",
              "save frame lookup");
        calls.emplace_back("find-frame"); return frame;
    }
    std::uint8_t IsVisible(std::uint32_t found) override {
        Check(found == 0x6000u, "visibility frame argument");
        calls.emplace_back("is-visible"); return visible;
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game"); return 0x7000u;
    }
    std::uint8_t InitializeStateAndUI(
        std::uint32_t game, std::uint32_t state) override {
        Check(game == 0x7000u && state == 1u,
              "game transition arguments");
        calls.emplace_back("transition"); return 0x42u;
    }
};

struct ExitRuntime final : pal4::ISaveUiExitRuntime {
    std::vector<std::string> calls;
    std::uint32_t manager = 0x8000u;
    std::uint32_t selected_index = 1u;
    std::uint32_t effect_count = 0u;

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager"); return manager;
    }
    std::uint32_t PlayerByIndex(
        std::uint32_t found_manager, std::uint32_t index) override {
        Check(found_manager == manager && index < 4u, "player lookup args");
        calls.push_back("player:" + std::to_string(index));
        if (manager == 0u || index == 1u) return 0u;
        return 0x10000u + index * 0x1000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        if (address == manager + 11344u) return selected_index;
        if (address == 0x10000u + 2824u ||
            address == 0x13000u + 2824u) return 1u;
        if (address == 0x12000u + 2824u) return 0u;
        throw std::runtime_error("unexpected player word read");
    }
    std::uint32_t GetComponentProperty(
        std::uint32_t component, std::uint32_t property) override {
        Check((component == 0x10000u + 1620u ||
               component == 0x13000u + 1620u) &&
              (property == 0u || property == 2u),
              "component property getter args");
        calls.push_back("get:" + std::to_string(property));
        return property == 0u ? 11u : 22u;
    }
    void SetComponentProperty(std::uint32_t component,
                              std::uint32_t property,
                              std::uint32_t value) override {
        Check((component == 0x10000u + 1620u ||
               component == 0x13000u + 1620u) &&
              ((property == 7u && value == 11u) ||
               (property == 9u && value == 22u)),
              "component property setter args");
        calls.push_back("set:" + std::to_string(property));
    }
    std::uint32_t EffectManager() override {
        calls.emplace_back("effect-manager"); return 0x9000u;
    }
    void CreateEffect(std::uint32_t effects,
                      std::string_view name,
                      std::uint32_t position,
                      std::uint32_t selector,
                      std::uint32_t flags) override {
        Check(effects == 0x9000u && name == "H_XS027" &&
              position == manager + 2836u * selected_index + 172u &&
              selector == 1u && flags == 0u,
              "selected player effect args");
        ++effect_count;
        calls.emplace_back("create-effect");
    }
    void ResetTemporaryEffectHandle() override {
        calls.emplace_back("reset-effect");
    }
};

void TestSaveState() {
    Runtime runtime;
    Check(pal4::SaveUiState::Construct(0x1000u, runtime) == 0x1000u &&
          runtime.words[0x1000u] == pal4::SaveUiState::kVtable &&
          runtime.calls == std::vector<std::string>{"write:4096"},
          "save state constructor");
    runtime.calls.clear();
    Check(pal4::SaveUiState::Enter(99u, 98u, runtime) == 1u &&
          runtime.words[0x3000u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "ui", "set-flag", "console", "write:12288",
              "ui", "show-save-ui"},
          "entry flag, console and event order");
    runtime.calls.clear();
    Check(pal4::SaveUiState::Render(0x5000u, runtime) == 0x77u &&
          runtime.calls == std::vector<std::string>{
              "read-frame", "arena", "process-world", "ui", "render-ui"},
          "world frame before UI render and return forwarding");
    runtime.calls.clear();
    Check(pal4::SaveUiState::Update(runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"ui", "find-frame"},
          "missing SaveWindow returns zero");
    runtime.calls.clear(); runtime.frame = 0x6000u; runtime.visible = 7u;
    Check(pal4::SaveUiState::Update(runtime) == 7u &&
          runtime.calls == std::vector<std::string>{
              "ui", "find-frame", "is-visible"},
          "visible SaveWindow returns raw visibility byte");
    runtime.calls.clear(); runtime.visible = 0u;
    Check(pal4::SaveUiState::Update(runtime) == 0x42u &&
          runtime.calls == std::vector<std::string>{
              "ui", "find-frame", "is-visible", "game", "transition"},
          "hidden SaveWindow transitions to game state one");
}

void TestFinish() {
    ExitRuntime runtime;
    Check(pal4::SaveUiState::Finish(99u, runtime) == 1u &&
          runtime.effect_count == 1u,
          "save UI finish return and effect");
    const std::vector<std::string> expected = {
        "manager", "player:0", "read:68360", "get:0", "set:7",
        "get:2", "set:9",
        "manager", "player:1",
        "manager", "player:2", "read:76552",
        "manager", "player:3", "read:80648", "get:0", "set:7",
        "get:2", "set:9",
        "manager", "read:44112", "manager", "effect-manager",
        "read:44112", "create-effect", "reset-effect"};
    Check(runtime.calls == expected,
          "all four players and repeated selected-player lookup order");
    runtime.calls.clear(); runtime.manager = 0u;
    runtime.selected_index = 0u;
    Check(pal4::SaveUiState::Finish(0u, runtime) == 1u &&
          runtime.effect_count == 1u &&
          runtime.calls == std::vector<std::string>{
              "manager", "player:0", "manager", "player:1",
              "manager", "player:2", "manager", "player:3",
              "manager", "read:11344"},
          "zero computed selected-player address skips effect");
}

}  // namespace

int main() {
    try {
        TestSaveState();
        TestFinish();
        std::cout << "save UI state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
