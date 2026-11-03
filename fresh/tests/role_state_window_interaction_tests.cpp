#include "pal4/role_state_window_interaction.h"

#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime final : pal4::IRoleStateWindowInteractionRuntime {
    std::map<std::uint32_t, std::uint32_t> words{
        {0x3254u, 2u}, {0x4008u, 0x7000u}};
    std::vector<std::string> calls;
    std::uint32_t capture = 0u;
    std::uint32_t root = 0x8000u;
    std::uint32_t child = 0x7000u;
    bool visible_result = false;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.push_back("read:" + std::to_string(address));
        return words[address];
    }
    std::uint32_t CapturedWindow() override {
        calls.emplace_back("capture");
        return capture;
    }
    std::uint32_t RootWindow() override {
        calls.emplace_back("root");
        return root;
    }
    std::uint32_t ChildAtPosition(std::uint32_t current_root,
                                  std::uint32_t position) override {
        Check(current_root == 0x8000u && position == 0x400cu,
              "hit test root and event position");
        calls.emplace_back("child");
        return child;
    }
    bool IsVisible(std::uint32_t window, bool recursive) override {
        Check(window == 0x9000u && !recursive,
              "visibility queries first role panel without recursion");
        calls.emplace_back("is-visible");
        return visible_result;
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        calls.push_back("visible:" + std::to_string(window) + ":" +
                        (visible ? "1" : "0"));
    }
    void SetImage(std::uint32_t window, std::string_view imageset,
                  std::string_view image) override {
        Check(imageset == "kuang0", "mouse button imageset");
        calls.push_back("image:" + std::to_string(window) + ":" +
                        std::string(image));
    }
    void ClearImage(std::uint32_t window) override {
        calls.push_back("clear:" + std::to_string(window));
    }
};

pal4::RoleStateWindowState State(std::uint32_t mode = 0u) {
    pal4::RoleStateWindowState state;
    state.Write(0u, 2u);
    state.Write(4u, mode);
    state.Write(12u, 0x9000u);
    state.Write(72u, 0x9100u);
    state.Write(76u, 0x9200u);
    state.Write(96u, 0x6000u);
    state.Write(100u, 0x7000u);
    state.Write(116u, 0x3000u);
    return state;
}

void TestMouseMoveGates() {
    Runtime runtime;
    auto state = State(1u);
    Check(pal4::RoleStateWindowInteraction::OnMouseMove(
              state, 0x4000u, runtime) && runtime.calls.empty(),
          "selected state stops before receiver read");

    state = State();
    runtime.words[0x3254u] = 1u;
    Check(pal4::RoleStateWindowInteraction::OnMouseMove(
              state, 0x4000u, runtime) &&
              runtime.calls == std::vector<std::string>{"read:12884"},
          "receiver count below two stops before capture check");
    runtime.calls.clear();
    runtime.words[0x3254u] = 0xffffffffu;
    Check(pal4::RoleStateWindowInteraction::OnMouseMove(
              state, 0x4000u, runtime) &&
              runtime.calls == std::vector<std::string>{"read:12884"},
          "receiver gate is signed");

    runtime.calls.clear();
    runtime.words[0x3254u] = 2u;
    runtime.capture = 0x9999u;
    Check(pal4::RoleStateWindowInteraction::OnMouseMove(
              state, 0x4000u, runtime) &&
              runtime.calls.back() == "capture" &&
              state.Read(4u) == 0u,
          "other captured window blocks hit test");
    runtime.calls.clear();
    runtime.capture = 0u;
    runtime.root = 0u;
    Check(pal4::RoleStateWindowInteraction::OnMouseMove(
              state, 0x4000u, runtime) &&
              runtime.calls.back() == "root" &&
              state.Read(4u) == 0u,
          "null CEGUI root blocks hit test");
    runtime.calls.clear();
    runtime.root = 0x8000u;
    runtime.child = 0x9999u;
    Check(pal4::RoleStateWindowInteraction::OnMouseMove(
              state, 0x4000u, runtime) &&
              runtime.calls.back() == "child" &&
              state.Read(4u) == 0u,
          "nonmatching child does not select role");
}

void TestMouseMoveAndLeave() {
    Runtime runtime;
    auto state = State();
    runtime.capture = 0x7000u;
    Check(pal4::RoleStateWindowInteraction::OnMouseMove(
              state, 0x4000u, runtime) && state.Read(4u) == 1u &&
              runtime.calls[runtime.calls.size() - 2u] ==
                  "image:24576:huangse" &&
              runtime.calls.back() == "image:28672:renwuanniu2c2",
          "matching capture and child select role artwork");
    runtime.calls.clear();
    Check(pal4::RoleStateWindowInteraction::OnMouseLeave(
              state, runtime) && state.Read(4u) == 0u &&
              runtime.calls == std::vector<std::string>{
                  "visible:24576:1", "clear:24576",
                  "image:28672:renwuanniuc"},
          "leave restores normal button artwork");
    runtime.calls.clear();
    Check(pal4::RoleStateWindowInteraction::OnMouseLeave(
              state, runtime) && runtime.calls.empty(),
          "leave from normal state does nothing");
    state.Write(4u, 2u);
    Check(pal4::RoleStateWindowInteraction::OnMouseLeave(
              state, runtime) && runtime.calls.empty(),
          "leave from hovered state does nothing");
}

void TestVisibility() {
    Runtime runtime;
    auto state = State();
    pal4::RoleStateWindowInteraction::SetVisible(state, true, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "visible:36864:1", "visible:37120:1",
              "visible:37376:0"},
          "show panel and details, hide curtain");
    runtime.calls.clear();
    pal4::RoleStateWindowInteraction::SetVisible(state, false, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "visible:36864:0", "visible:37120:0",
              "visible:37376:1"},
          "hide panel and details, show curtain");
    runtime.calls.clear();
    Check(!pal4::RoleStateWindowInteraction::IsVisible(state, runtime) &&
              runtime.calls == std::vector<std::string>{"is-visible"},
          "query hidden role panel");
    runtime.calls.clear();
    runtime.visible_result = true;
    Check(pal4::RoleStateWindowInteraction::IsVisible(state, runtime) &&
              runtime.calls == std::vector<std::string>{"is-visible"},
          "query visible role panel");
}

}  // namespace

int main() {
    TestMouseMoveGates();
    TestMouseMoveAndLeave();
    TestVisibility();
}
