#include "pal4/magic_window_character_display.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

void Check(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(EXIT_FAILURE); }
}

struct Runtime final : pal4::MagicWindowCharacterDisplayRuntimeStub {
    std::array<std::optional<pal4::MagicPlayerSnapshot>, 4> players{};
    std::vector<std::string> calls;
    std::string portrait;
    std::uint32_t portrait_window{};
    bool left_visible{};
    bool right_visible{};

    std::optional<pal4::MagicPlayerSnapshot> GetPlayer(std::int32_t index) override {
        calls.push_back("player:" + std::to_string(index));
        if (index < 0 || index >= 4) return std::nullopt;
        return players[static_cast<std::size_t>(index)];
    }
    void SetPortrait(std::uint32_t window, std::string_view image_set,
                     std::string_view image_name) override {
        Check(image_set == "zi0", "portrait image set");
        portrait_window = window;
        portrait = image_name;
        calls.push_back("portrait");
    }
    void InitializeMagicImages() override { calls.push_back("initialize-images"); }
    void UpdateMagicImages() override { calls.push_back("update-images"); }
    void SetVisible(std::uint32_t window, bool visible) override {
        if (window == 0x2000u) left_visible = visible;
        else if (window == 0x3000u) right_visible = visible;
        else Check(false, "navigation button target");
        calls.push_back(window == 0x2000u ? "left" : "right");
    }
};

pal4::MagicWindowPanelState MakePanel() {
    pal4::MagicWindowPanelState panel;
    panel.At(0x7cu) = 0x1000u;
    panel.At(0x80u) = 0x2000u;
    panel.At(0x84u) = 0x3000u;
    return panel;
}

void TestMissingPlayer() {
    pal4::MagicWindowState state;
    Runtime runtime;
    auto panel = MakePanel();
    pal4::MagicWindowCharacterDisplay::Update(state, panel, -1, runtime);
    Check(state.selected_character_index == -1 &&
              runtime.calls == std::vector<std::string>{"player:-1"},
          "selection stored even without player and no UI work follows");
}

void TestPortraitsAndInitialization() {
    constexpr std::array<std::string_view, 4> names{
        "yuntianhe", "hanlingsha", "liumengli", "murongziying"};
    auto panel = MakePanel();
    for (std::uint32_t portrait_id = 0; portrait_id < names.size(); ++portrait_id) {
        pal4::MagicWindowState state;
        state.category_position = 1u;
        Runtime runtime;
        runtime.players[0] = pal4::MagicPlayerSnapshot{portrait_id, true};
        runtime.players[1] = pal4::MagicPlayerSnapshot{0u, true};
        pal4::MagicWindowCharacterDisplay::Update(state, panel, 0, runtime);
        Check(state.selected_character_index == 0 &&
                  runtime.portrait_window == 0x1000u &&
                  runtime.portrait == names[portrait_id] &&
                  runtime.left_visible && runtime.right_visible,
              "four role images and multi-player navigation");
        Check(runtime.calls == std::vector<std::string>{
                  "player:0", "portrait", "initialize-images", "player:0",
                  "player:1", "player:2", "player:3", "left", "right"},
              "portrait, magic and player-count order");
    }
}

void TestUpdateAndNavigationBoundaries() {
    auto panel = MakePanel();
    pal4::MagicWindowState state;
    Runtime runtime;
    runtime.players[2] = pal4::MagicPlayerSnapshot{99u, false};
    pal4::MagicWindowCharacterDisplay::Update(state, panel, 2, runtime);
    Check(runtime.portrait.empty() && !runtime.left_visible &&
              !runtime.right_visible &&
              runtime.calls[1] == "update-images",
          "unknown portrait still refreshes images and hides navigation");

    runtime = Runtime{};
    runtime.players[2] = pal4::MagicPlayerSnapshot{2u, true};
    pal4::MagicWindowCharacterDisplay::Update(state, panel, 2, runtime);
    Check(!runtime.left_visible && !runtime.right_visible,
          "one active player hides both buttons");

    runtime = Runtime{};
    runtime.players[2] = pal4::MagicPlayerSnapshot{2u, true};
    runtime.players[3] = pal4::MagicPlayerSnapshot{3u, true};
    pal4::MagicWindowCharacterDisplay::Update(state, panel, 2, runtime);
    Check(runtime.left_visible && runtime.right_visible,
          "two active players show both buttons");
}

}  // namespace

int main() {
    TestMissingPlayer();
    TestPortraitsAndInitialization();
    TestUpdateAndNavigationBoundaries();
}
