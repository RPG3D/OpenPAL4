#include "pal4/logo_state.h"

#include <algorithm>
#include <array>
#include <cmath>
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

std::size_t Position(const std::vector<std::string>& calls,
                     const std::string& name) {
    return static_cast<std::size_t>(
        std::find(calls.begin(), calls.end(), name) - calls.begin());
}

struct Runtime final : pal4::ILogoStateRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::map<std::uint32_t, float> floats;
    std::vector<std::string> calls;
    bool playing = false;
    bool needs_update = true;

    Runtime() { words[0x4000u + 412u] = 0x9000u; }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read-word"); return words[address];
    }
    void WriteWord(std::uint32_t address, std::uint32_t value) override {
        words[address] = value;
        calls.push_back("write:" + std::to_string(address));
    }
    float ReadFloat(std::uint32_t address) override {
        Check(address == 0x1000u + 12u, "timer read offset");
        calls.emplace_back("read-timer"); return floats[address];
    }
    void WriteFloat(std::uint32_t address, float value) override {
        Check(address == 0x1000u + 12u, "timer write offset");
        floats[address] = value; calls.emplace_back("write-timer");
    }
    std::uint32_t GlobalData() override {
        calls.emplace_back("global-data"); return 1u;
    }
    void SaveDialogSettings(std::uint32_t global) override {
        Check(global == 1u, "global data argument");
        calls.emplace_back("save-dialog-settings");
    }
    std::uint32_t UIFrameManager() override {
        calls.emplace_back("ui-manager"); return 0x4000u;
    }
    void SetUIFlag(std::uint32_t manager, std::uint32_t value) override {
        Check(manager == 0x4000u && value == 1u, "logo UI flag");
        calls.emplace_back("ui-flag");
    }
    std::uint32_t MusicPlayer() override {
        calls.emplace_back("music-player"); return 2u;
    }
    void FadeInMusic(std::uint32_t player, std::string_view track,
                     double seconds, std::uint32_t flags) override {
        Check(player == 2u && track == "P01-1.mp3" && seconds == 3.0 &&
              flags == 0u, "logo music fade arguments");
        calls.emplace_back("fade-music");
    }
    void AddUIElement(std::uint32_t manager, std::string_view name) override {
        Check(manager == 0x4000u && name == "ZJM", "logo UI element");
        calls.emplace_back("add-element");
    }
    void SetupUICamera(std::uint32_t camera, std::uint32_t enabled) override {
        Check(camera == 0x9000u && enabled == 1u, "UI camera setup");
        calls.emplace_back("setup-ui-camera");
    }
    void SendUIEvent(std::uint32_t manager, std::uint32_t event,
                     std::uint32_t first, std::uint32_t second) override {
        Check(manager == 0x4000u && second == 0u, "logo UI event payload");
        if (event == pal4::LogoState::kOpenUIEvent) {
            Check(first == 1u, "open event first word");
            calls.emplace_back("open-event");
        } else {
            Check(event == pal4::LogoState::kCloseUIEvent && first == 0u,
                  "close event descriptor and payload");
            calls.emplace_back("close-event");
        }
    }
    std::uint32_t CameraManager() override {
        calls.emplace_back("camera-manager"); return 3u;
    }
    std::uint32_t UICameraName() override { return 4u; }
    void ActivateCamera(std::uint32_t manager, std::uint32_t name) override {
        Check(manager == 3u && name == 4u, "UI camera activation");
        calls.emplace_back("activate-camera");
    }
    void ToggleWindows(std::uint32_t manager, bool enabled) override {
        Check(manager == 0x4000u, "toggle windows manager");
        calls.emplace_back(enabled ? "windows-on" : "windows-off");
    }
    bool EffectNeedsUpdate(std::uint32_t handle) override {
        Check(handle == 0x1004u, "effect handle offset");
        calls.emplace_back("effect-needs-update"); return needs_update;
    }
    void HandleEffect(std::uint32_t handle, std::uint32_t mode) override {
        Check(handle == 0x1004u && mode == 0u, "effect event arguments");
        calls.emplace_back("handle-effect");
    }
    pal4::LogoEffectHandle CreateEffect(
        std::uint32_t manager, std::string_view name,
        const std::array<float, 3>& position,
        std::uint32_t selector, std::uint32_t flags) override {
        Check(manager == 0x4000u && name == "ZJM" &&
              position == std::array<float, 3>{16.0f, 209.0f, -70.0f} &&
              selector == 0xffffffffu && flags == 0u,
              "logo effect creation arguments");
        calls.emplace_back("create-effect");
        return {0x33u, 0x44u};
    }
    void ResetTemporaryEffectHandle() override {
        calls.emplace_back("reset-temporary-effect");
    }
    void SetEffectProperties(
        std::uint32_t handle, const std::array<float, 3>& scale,
        std::uint8_t enabled) override {
        Check(handle == 0x1004u &&
              scale == std::array<float, 3>{1.2f, 1.2f, 1.2f} &&
              enabled == 0u, "logo effect scale arguments");
        calls.emplace_back("set-effect-scale");
    }
    std::uint32_t BinkPlayer() override {
        calls.emplace_back("bink-player"); return 0x5000u;
    }
    bool IsBinkPlaying(std::uint32_t player_state) override {
        Check(player_state == 0x5004u, "Bink player state offset");
        calls.emplace_back("bink-playing"); return playing;
    }
    void PreRenderUI(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 0x4000u && frame == 0x6000u, "UI pre-render args");
        calls.emplace_back("pre-render");
    }
    std::uint32_t ScreenEffect() override {
        calls.emplace_back("screen-effect"); return 6u;
    }
    void DrawScreenEffect(std::uint32_t effect,
                          std::uint32_t frame) override {
        Check(effect == 6u && frame == 0x6000u, "screen effect draw args");
        calls.emplace_back("draw-effect");
    }
    void AdvanceScreenEffect(std::uint32_t effect, float delta) override {
        Check(effect == 6u && delta == 0.25f, "screen effect delta");
        calls.emplace_back("advance-effect");
    }
    void RenderBink(std::uint32_t player, std::uint32_t frame) override {
        Check(player == 0x5000u && frame == 0x6000u, "Bink render args");
        calls.emplace_back("render-bink");
    }
    void RenderUI(std::uint32_t manager, std::uint32_t frame) override {
        Check(manager == 0x4000u && frame == 0x6000u, "UI render args");
        calls.emplace_back("render-ui");
    }
    std::uint32_t Console() override {
        calls.emplace_back("console"); return 7u;
    }
    void RenderConsole(std::uint32_t console, std::uint32_t frame) override {
        Check(console == 7u && frame == 0x6000u, "console render args");
        calls.emplace_back("render-console");
    }
    std::uint32_t GameHeight() override { return 480u; }
    void DrawVersionText(std::string_view text,
                         std::int32_t x, std::int32_t y,
                         std::int32_t width, std::int32_t height,
                         std::uint32_t mode, std::uint32_t color,
                         std::uint32_t flags) override {
        Check(text == "PAL4 v1.1" && x == 20 && y == 400 &&
              width == 200 && height == 20 && mode == 1u &&
              color == 0xffefefefu && flags == 16u,
              "version font and rectangle");
        calls.emplace_back("draw-version");
    }
    void ProcessScriptWaitHandlers(double delta) override {
        Check(delta == 0.25, "logo script wait delta");
        calls.emplace_back("process-script-waits");
    }
    void UpdateUI(std::uint32_t manager, double delta) override {
        Check(manager == 0x4000u && delta == 0.25, "logo UI update args");
        calls.emplace_back("update-ui");
    }
    std::uint32_t UpdateEffects(
        std::uint32_t manager, double delta) override {
        Check(manager == 0x4000u && delta == 0.25,
              "logo transform update args");
        calls.emplace_back("effect-transform"); return 0x88u;
    }
    void CleanupEffects(std::uint32_t manager) override {
        Check(manager == 0x4000u, "logo cleanup manager");
        calls.emplace_back("cleanup-effects");
    }
};

void TestEntry() {
    Runtime runtime;
    Check(pal4::LogoState::Enter(0x1000u, runtime) == 1u,
          "logo entry return");
    Check(runtime.words[0x1004u] == 0x33u &&
          runtime.words[0x1008u] == 0x44u &&
          runtime.words[0x100cu] == 0x40000000u,
          "logo effect handle and timer");
    const std::vector<std::string> order = {
        "save-dialog-settings", "ui-flag", "fade-music", "add-element",
        "setup-ui-camera", "open-event", "activate-camera", "windows-on",
        "handle-effect", "create-effect", "reset-temporary-effect",
        "set-effect-scale"};
    for (std::size_t i = 1; i < order.size(); ++i)
        Check(Position(runtime.calls, order[i - 1]) <
                  Position(runtime.calls, order[i]), "logo entry order");
    Check(std::count(runtime.calls.begin(), runtime.calls.end(),
                     "effect-needs-update") == 2,
          "effect needs-update queried before and after creation");
    runtime.calls.clear(); runtime.needs_update = false;
    Check(pal4::LogoState::Enter(0x1000u, runtime) == 1u &&
          Position(runtime.calls, "handle-effect") == runtime.calls.size() &&
          Position(runtime.calls, "create-effect") < runtime.calls.size(),
          "old effect event conditional");
}

void TestFrameAndLeave() {
    Runtime runtime;
    runtime.floats[0x100cu] = 0.0f;
    pal4::LogoState::Render(0x1000u, 0x6000u, runtime);
    const std::vector<std::string> order = {
        "bink-playing", "pre-render", "draw-effect", "render-bink",
        "render-ui", "render-console", "draw-version"};
    for (std::size_t i = 1; i < order.size(); ++i)
        Check(Position(runtime.calls, order[i - 1]) <
                  Position(runtime.calls, order[i]), "logo render order");
    runtime.calls.clear(); runtime.playing = true;
    runtime.floats[0x100cu] = 1.0f;
    pal4::LogoState::Render(0x1000u, 0x6000u, runtime);
    Check(Position(runtime.calls, "pre-render") == runtime.calls.size() &&
          Position(runtime.calls, "draw-version") == runtime.calls.size() &&
          Position(runtime.calls, "render-console") < runtime.calls.size(),
          "video and timer render gates");
    runtime.calls.clear(); runtime.floats[0x100cu] = 0.5f;
    Check(pal4::LogoState::Update(0x1000u, 0.25f, 99u, 0u, runtime) == 0x88u &&
          runtime.floats[0x100cu] == 0.25f &&
          Position(runtime.calls, "write-timer") < Position(runtime.calls, "process-script-waits") &&
          Position(runtime.calls, "process-script-waits") < Position(runtime.calls, "advance-effect") &&
          Position(runtime.calls, "advance-effect") < Position(runtime.calls, "update-ui") &&
          Position(runtime.calls, "update-ui") < Position(runtime.calls, "effect-transform"),
          "logo update order and effect gate");
    runtime.calls.clear(); runtime.floats[0x100cu] = 0.1f;
    Check(pal4::LogoState::Update(0x1000u, 0.25f, 0u, 1u, runtime) == 0x88u &&
          runtime.floats[0x100cu] == 0.0f &&
          Position(runtime.calls, "advance-effect") == runtime.calls.size(),
          "timer clamp and effect suppression");
    runtime.calls.clear();
    Check(pal4::LogoState::LeaveWrapper(0x1000u, 99u, runtime) == 1u &&
          Position(runtime.calls, "handle-effect") < Position(runtime.calls, "close-event") &&
          Position(runtime.calls, "close-event") < Position(runtime.calls, "windows-off") &&
          Position(runtime.calls, "windows-off") < Position(runtime.calls, "cleanup-effects"),
          "logo leave wrapper and service order");
}

}  // namespace

int main() {
    try {
        TestEntry();
        TestFrameAndLeave();
        std::cout << "logo state tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
