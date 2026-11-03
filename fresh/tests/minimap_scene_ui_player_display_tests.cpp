#include "pal4/minimap_scene_ui_player_display.h"

#include <array>
#include <bit>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Assignment {
    std::uint32_t button;
    pal4::MinimapButtonImageState state;
    std::uint32_t image;
};

struct Runtime final : pal4::IMinimapSceneUiPlayerDisplayRuntime {
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::vector<Assignment> assignments;
    std::vector<std::uint32_t> icon_actions;
    std::int32_t mode = 1;
    std::uint32_t action = 0;
    std::uint32_t fourth_action = 0;
    std::uint32_t action_reads = 0;
    int fail_lookup = 0;
    pal4::MinimapFlashColor color{};

    std::uint32_t PlayerManager() override {
        calls.emplace_back("manager");
        return 0x7000u;
    }
    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        if (address == 0x9c50u) {
            return 2u;
        }
        Check(address == 0x7000u + 2836u * 2u + 0xa48u,
              "selected player action address");
        ++action_reads;
        return action_reads == 4u ? fourth_action : action;
    }
    std::uint32_t GetIconImage(std::uint32_t requested_action,
                               std::uint32_t variant,
                               std::uint32_t flags) override {
        Check(variant >= 3u && variant <= 5u && flags == 0u,
              "icon variant and flags");
        icon_actions.push_back(requested_action);
        calls.emplace_back("icon");
        return 0x100u + variant;
    }
    std::uint32_t GetImage(std::string_view imageset,
                           std::string_view name) override {
        Check(imageset == "zhujiemian0", "map imageset");
        names.emplace_back(name);
        calls.emplace_back("image");
        if (fail_lookup != 0 &&
            static_cast<int>(names.size()) == fail_lookup) {
            throw std::runtime_error("image lookup failed");
        }
        return 0x200u + static_cast<std::uint32_t>(names.size());
    }
    void SetButtonImage(std::uint32_t button,
                        std::string_view part,
                        pal4::MinimapButtonImageState image_state,
                        std::uint32_t image) override {
        Check((button == 0x9000u || button == 0xa000u) && part == "Self",
              "button image target and part");
        assignments.push_back({button, image_state, image});
        calls.emplace_back("set");
    }
    std::int32_t PlayerMode(std::uint32_t player) override {
        Check(player == 0x7000u + 2836u * 2u,
              "player mode from selected player");
        calls.emplace_back("mode");
        return mode;
    }
    void ApplyColor(std::uint32_t button,
                    pal4::MinimapFlashColor value) override {
        Check(button == 0x9000u, "icon button color target");
        color = value;
        calls.emplace_back("color");
    }
    void RequestRedraw(std::uint32_t button) override {
        Check(button == 0x9000u, "icon button redraw target");
        calls.emplace_back("redraw");
    }
};

pal4::MinimapSceneUiState MakeState() {
    pal4::MinimapSceneUiState state;
    state.WriteWord(0x68u, 0xa000u);
    state.WriteWord(0xb4u, 0x9000u);
    return state;
}

void CheckAssignments(const Runtime& runtime, bool has_map_images) {
    const std::array states{
        pal4::MinimapButtonImageState::Normal,
        pal4::MinimapButtonImageState::Highlight,
        pal4::MinimapButtonImageState::Pushed,
    };
    Check(runtime.assignments.size() == (has_map_images ? 6u : 3u),
          "icon and optional map assignment count");
    for (std::size_t index = 0; index < runtime.assignments.size(); ++index) {
        const auto& assignment = runtime.assignments[index];
        Check(assignment.button == (index < 3u ? 0x9000u : 0xa000u) &&
                  assignment.state == states[index % 3u] &&
                  assignment.image == (index < 3u
                                           ? 0x103u + static_cast<std::uint32_t>(index)
                                           : 0x201u + static_cast<std::uint32_t>(index - 3u)),
              "normal/highlight/pushed image order");
    }
}

void CheckColors() {
    const auto state = MakeState();
    for (const auto mode : {-1, 0, 1, 2, 3}) {
        Runtime runtime;
        runtime.mode = mode;
        pal4::MinimapSceneUiPlayerDisplay::Update(state, runtime);
        const auto red = std::bit_cast<std::uint32_t>(runtime.color.red);
        const auto green = std::bit_cast<std::uint32_t>(runtime.color.green);
        const auto blue = std::bit_cast<std::uint32_t>(runtime.color.blue);
        Check(red == (mode == 1 ? 0x3f139394u
                                : mode == 2 ? 0x3f33b3b4u : 0x3f800000u) &&
                  green == (mode == 1 ? 0x3f39b9bau
                                  : mode == 2 ? 0x3f4ececfu : 0x3f800000u) &&
                  blue == (mode == 1 ? 0x3f5ededfu
                                 : mode == 2 ? 0x3f69e9eau : 0x3f800000u) &&
                  runtime.color.alpha == 1.0f &&
                  runtime.calls[runtime.calls.size() - 3u] == "mode" &&
                  runtime.calls[runtime.calls.size() - 2u] == "color" &&
                  runtime.calls.back() == "redraw",
              "mode color bits and redraw order");
    }
}

}  // namespace

int main() {
    const auto state = MakeState();
    const std::array<std::array<std::string, 3>, 4> expected{{
        {{"dituanniuxiangzi1", "dituanniuxiangzi1_2", "dituanniuxiangzi1"}},
        {{"dituanniuxiangzi2", "dituanniuxiangzi2_2", "dituanniuxiangzi2"}},
        {{"dituanniuxiangzi4", "dituanniuxiangzi4_2", "dituanniuxiangzi4"}},
        {{"dituanniuxiangzi3", "dituanniuxiangzi3_2", "dituanniuxiangzi3"}},
    }};
    for (std::uint32_t action = 0u; action < expected.size(); ++action) {
        Runtime runtime;
        runtime.action = action;
        runtime.fourth_action = action;
        pal4::MinimapSceneUiPlayerDisplay::Update(state, runtime);
        Check(runtime.names == std::vector<std::string>(
                                   expected[action].begin(), expected[action].end()) &&
                  runtime.icon_actions ==
                      std::vector<std::uint32_t>{action, action, action} &&
                  runtime.action_reads == 4u &&
                  runtime.calls.front() == "manager" &&
                  runtime.calls[1] == "read",
              "selected player, action rereads and exact map image names");
        CheckAssignments(runtime, true);
    }
    for (const auto action : {4u, 0xffffffffu}) {
        Runtime runtime;
        runtime.action = action;
        runtime.fourth_action = action;
        pal4::MinimapSceneUiPlayerDisplay::Update(state, runtime);
        Check(runtime.names.empty() && runtime.action_reads == 4u,
              "unsupported action omits map images");
        CheckAssignments(runtime, false);
    }
    {
        Runtime runtime;
        runtime.action = 0u;
        runtime.fourth_action = 2u;
        pal4::MinimapSceneUiPlayerDisplay::Update(state, runtime);
        Check(runtime.names == std::vector<std::string>(
                                   expected[2].begin(), expected[2].end()) &&
                  runtime.icon_actions ==
                      std::vector<std::uint32_t>{0u, 0u, 0u},
              "map branch rereads the action after three icon queries");
    }
    {
        Runtime runtime;
        runtime.fail_lookup = 2;
        bool caught = false;
        try {
            pal4::MinimapSceneUiPlayerDisplay::Update(state, runtime);
        } catch (const std::runtime_error&) {
            caught = true;
        }
        Check(caught && runtime.assignments.size() == 4u &&
                  runtime.calls.back() == "image",
              "image lookup failure stops before mode, color and redraw");
    }
    CheckColors();
}
