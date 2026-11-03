#include "pal4/minimap_scene_ui_movement_display.h"

#include <array>
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

struct Runtime final : pal4::IMinimapSceneUiMovementDisplayRuntime {
    struct Assignment {
        pal4::MinimapButtonImageState state;
        std::uint32_t image;
    };
    std::vector<std::string> calls;
    std::vector<std::string> names;
    std::vector<Assignment> assignments;
    std::int32_t mode = 0;
    int fail_lookup = 0;

    std::uint32_t ActorControl() override {
        calls.emplace_back("control");
        return 0x9000u;
    }
    std::int32_t ActorControlMode(std::uint32_t control) override {
        Check(control == 0x9000u, "movement control pointer");
        calls.emplace_back("mode");
        return mode;
    }
    std::uint32_t GetImage(std::string_view imageset,
                           std::string_view name) override {
        Check(imageset == "zhujiemian0", "movement imageset");
        names.emplace_back(name);
        calls.emplace_back("lookup");
        if (fail_lookup != 0 &&
            static_cast<int>(names.size()) == fail_lookup)
            throw std::runtime_error("lookup");
        return 0xa000u + static_cast<std::uint32_t>(names.size());
    }
    void SetButtonImage(std::uint32_t button,
                        std::string_view part,
                        pal4::MinimapButtonImageState state,
                        std::uint32_t image) override {
        Check(button == 0x7000u && part == "Self",
              "run button and inner part");
        assignments.push_back({state, image});
        calls.emplace_back("set-image");
    }
    void RequestRedraw(std::uint32_t button) override {
        Check(button == 0x7000u, "run button redraw target");
        calls.emplace_back("redraw");
    }
};

pal4::MinimapSceneUiState MakeState() {
    pal4::MinimapSceneUiState state;
    state.WriteWord(0xb8u, 0x7000u);
    return state;
}

void CheckModes() {
    const auto state = MakeState();
    const std::array<std::array<std::string, 3>, 3> expected{{
        {{"zou", "zou2", "zou3"}},
        {{"pao", "pao2", "pao3"}},
        {{"kuaipao", "kuaipao2", "kuaipao3"}},
    }};
    for (std::int32_t mode = 0; mode < 3; ++mode) {
        Runtime runtime;
        runtime.mode = mode;
        pal4::MinimapSceneUiMovementDisplay::Update(state, runtime);
        Check(runtime.names == std::vector<std::string>(
                                  expected[static_cast<std::size_t>(mode)].begin(),
                                  expected[static_cast<std::size_t>(mode)].end()) &&
                  runtime.assignments.size() == 4u &&
                  runtime.assignments[0].state ==
                      pal4::MinimapButtonImageState::Normal &&
                  runtime.assignments[0].image == 0xa001u &&
                  runtime.assignments[1].state ==
                      pal4::MinimapButtonImageState::Highlight &&
                  runtime.assignments[1].image == 0xa002u &&
                  runtime.assignments[2].state ==
                      pal4::MinimapButtonImageState::Disabled &&
                  runtime.assignments[2].image == 0xa003u &&
                  runtime.assignments[3].state ==
                      pal4::MinimapButtonImageState::Pushed &&
                  runtime.assignments[3].image == 0xa003u &&
                  runtime.calls.back() == "redraw",
              "three movement modes map images to four button states");
    }
}

void CheckUnknownAndFailure() {
    const auto state = MakeState();
    for (auto mode : {-1, 3, 99}) {
        Runtime runtime;
        runtime.mode = mode;
        pal4::MinimapSceneUiMovementDisplay::Update(state, runtime);
        Check(runtime.names.empty() && runtime.assignments.empty() &&
                  runtime.calls == std::vector<std::string>{
                                       "control", "mode", "redraw"},
              "unsupported mode still requests button redraw");
    }

    Runtime failure;
    failure.fail_lookup = 2;
    bool caught = false;
    try {
        pal4::MinimapSceneUiMovementDisplay::Update(state, failure);
    } catch (const std::runtime_error&) {
        caught = true;
    }
    Check(caught && failure.names.size() == 2u &&
              failure.assignments.size() == 1u &&
              failure.calls.back() == "lookup",
          "image lookup failure stops later assignments and redraw");
}

}  // namespace

int main() {
    CheckModes();
    CheckUnknownAndFailure();
}
