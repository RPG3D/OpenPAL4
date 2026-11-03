#include "pal4/ui_frame_transition_state.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IUiFrameTransitionRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::optional<std::uint32_t> configured_frame;
    std::uint8_t visible = 1u;
    std::uint8_t transition_result = 0x42u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read:" + std::to_string(address));
        return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.emplace_back("write:" + std::to_string(address));
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui"); return 0x3000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x3000u && value == 1u, "frame UI flag");
        calls.emplace_back("set-flag");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 0x4000u;
    }
    std::optional<std::uint32_t> ReadIntegerAttribute(
        std::uint32_t config, std::string_view key) override {
        Check(config == 0x5000u && key == "frame", "frame config key");
        calls.emplace_back("read-frame-config"); return configured_frame;
    }
    std::uint8_t IsDialogVisible(std::uint32_t frame) override {
        Check(frame == 0x6000u, "dialog frame argument");
        calls.emplace_back("is-visible"); return visible;
    }
    std::uint32_t GameInstance() override {
        calls.emplace_back("game"); return 0x7000u;
    }
    std::uint8_t InitializeStateAndUI(
        std::uint32_t game, std::uint32_t state) override {
        Check(game == 0x7000u && state == 1u, "game transition args");
        calls.emplace_back("transition"); return transition_result;
    }
};

void TestLifecycle() {
    Runtime runtime;
    Check(pal4::UiFrameTransitionState::Construct(0x1000u, runtime) == 0x1000u &&
          runtime.words[0x1000u] == pal4::UiFrameTransitionState::kVtable &&
          runtime.words[0x1004u] == 0u &&
          runtime.calls == std::vector<std::string>{"write:4096", "write:4100"},
          "frame state constructor");
    runtime.calls.clear();
    Check(pal4::UiFrameTransitionState::LoadConfig(
              0x1000u, 99u, 0x5000u, runtime) == 1u &&
          runtime.words[0x1004u] == 0u &&
          runtime.calls == std::vector<std::string>{
              "ui", "set-flag", "console", "write:16384",
              "read-frame-config", "write:4100"},
          "missing frame defaults to zero");
    runtime.calls.clear(); runtime.configured_frame = 0x6000u;
    Check(pal4::UiFrameTransitionState::LoadConfig(
              0x1000u, 0u, 0x5000u, runtime) == 1u &&
          runtime.words[0x1004u] == 0x6000u,
          "configured frame stored");
    runtime.calls.clear(); runtime.visible = 7u;
    Check(pal4::UiFrameTransitionState::Update(0x1000u, runtime) == 7u &&
          runtime.calls == std::vector<std::string>{
              "read:4100", "is-visible"},
          "visible frame returns raw low byte without transition");
    runtime.calls.clear(); runtime.visible = 0u;
    Check(pal4::UiFrameTransitionState::Update(0x1000u, runtime) == 0x42u &&
          runtime.calls == std::vector<std::string>{
              "read:4100", "is-visible", "game", "transition"},
          "hidden frame transitions to game state one");
    runtime.calls.clear();
    Check(pal4::UiFrameTransitionState::Reset(0x1000u, 99u, runtime) == 1u &&
          runtime.words[0x1004u] == 0u &&
          runtime.calls == std::vector<std::string>{"write:4100"},
          "reset clears frame pointer");
    runtime.calls.clear();
    Check(pal4::UiFrameTransitionState::Update(0x1000u, runtime) == 0x42u &&
          runtime.calls == std::vector<std::string>{
              "read:4100", "game", "transition"},
          "zero frame transitions without visibility call");
}

}  // namespace

int main() {
    try {
        TestLifecycle();
        std::cout << "UI frame transition state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
