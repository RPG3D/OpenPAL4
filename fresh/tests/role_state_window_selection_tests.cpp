#include "pal4/role_state_window_selection.h"

#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IRoleStateWindowSelectionRuntime {
    std::vector<std::string> calls;
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x6000u && visible,
              "white background becomes visible");
        calls.emplace_back("visible");
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(imageset == "kuang0", "imageset literal");
        calls.push_back("image:" + std::to_string(window) + ":" +
                        std::string(image));
    }
    void ClearImage(std::uint32_t window) override {
        Check(window == 0x6000u, "clear background image");
        calls.emplace_back("clear-image");
    }
};

void TestRole(std::uint32_t role, std::string_view image) {
    pal4::RoleStateWindowState state;
    state.Write(0u, role);
    state.Write(4u, 0xaabbccdd);
    state.Write(96u, 0x6000u);
    state.Write(100u, 0x7000u);
    Runtime runtime;
    pal4::RoleStateWindowSelection::SetRoleState(state, runtime);
    Check(state.Read(4u) == 1u &&
              runtime.calls == std::vector<std::string>{
                  "visible", "image:24576:huangse",
                  "image:28672:" + std::string(image)},
          "role image sequence and state flag");
}

void TestButton(std::uint32_t role, std::string_view image) {
    pal4::RoleStateWindowState state;
    state.Write(0u, role);
    state.Write(4u, 0xaabbccdd);
    state.Write(96u, 0x6000u);
    state.Write(100u, 0x7000u);
    Runtime runtime;
    pal4::RoleStateWindowSelection::SetButtonImage(state, runtime);
    Check(state.Read(4u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "visible", "clear-image",
                  "image:28672:" + std::string(image)},
          "normal button image sequence and state flag");
}

void TestHovered(std::uint32_t role, std::string_view image) {
    pal4::RoleStateWindowState state;
    state.Write(0u, role);
    state.Write(4u, 0xaabbccdd);
    state.Write(96u, 0x6000u);
    state.Write(100u, 0x7000u);
    Runtime runtime;
    pal4::RoleStateWindowSelection::SetHoveredButtonImage(state, runtime);
    Check(state.Read(4u) == 2u &&
              runtime.calls == std::vector<std::string>{
                  "visible", "image:24576:huose",
                  "image:28672:" + std::string(image)},
          "hovered button image sequence and state flag");
}

}  // namespace

int main() {
    TestRole(0u, "renwuanniu2");
    TestRole(1u, "renwuanniu2b2");
    TestRole(2u, "renwuanniu2c2");
    TestRole(3u, "renwuanniu2d2");
    TestButton(0u, "renwuanniu1");
    TestButton(1u, "renwuanniub");
    TestButton(2u, "renwuanniuc");
    TestButton(3u, "renwuanniud");
    TestHovered(0u, "renwuanniu3");
    TestHovered(1u, "renwuanniu3b3");
    TestHovered(2u, "renwuanniu3c3");
    TestHovered(3u, "renwuanniu3d3");

    for (std::uint32_t role : {4u, 0xffffffffu}) {
        pal4::RoleStateWindowState state;
        state.Write(0u, role);
        state.Write(96u, 0x6000u);
        state.Write(100u, 0x7000u);
        Runtime runtime;
        pal4::RoleStateWindowSelection::SetRoleState(state, runtime);
        Check(state.Read(4u) == 1u &&
                  runtime.calls == std::vector<std::string>{
                      "visible", "image:24576:huangse"},
              "invalid role updates only shared background");
        runtime.calls.clear();
        pal4::RoleStateWindowSelection::SetButtonImage(state, runtime);
        Check(state.Read(4u) == 0u &&
                  runtime.calls == std::vector<std::string>{
                      "visible", "clear-image"},
              "invalid role clears shared image without selecting button");
        runtime.calls.clear();
        pal4::RoleStateWindowSelection::SetHoveredButtonImage(
            state, runtime);
        Check(state.Read(4u) == 2u &&
                  runtime.calls == std::vector<std::string>{
                      "visible", "image:24576:huose"},
              "invalid role sets hovered background only");
    }
}
