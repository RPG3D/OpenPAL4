#include "pal4/introduction_window_visibility.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

struct Runtime : pal4::IntroductionWindowVisibilityRuntimeStub {
    std::vector<char> calls;
    std::vector<std::uint32_t> enabled;
    std::uint32_t parent_showing{};

    void SetParent(pal4::IntroductionWindowState&,
                   std::uint32_t showing,
                   std::uint32_t parent) override {
        Check(parent == 0x7000u,
              "dialog parent receives original parent window");
        parent_showing = showing;
        calls.push_back('P');
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0xac00u && !visible,
              "PanelInfo is hidden before insertion");
        calls.push_back('V');
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        Check(value, "four selector buttons become enabled");
        enabled.push_back(window);
        calls.push_back('E');
    }
    std::uint32_t FrameManager() override {
        calls.push_back('M');
        return 0x9000u;
    }
    void InsertActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x9134u && member == 0x1028u,
              "show inserts object +0x28 into manager +0x134");
        calls.push_back('I');
    }
    void RemoveActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x9134u && member == 0x1028u,
              "hide removes the same active member");
        calls.push_back('R');
    }
    void SetPanelVisibilityAndText(
        pal4::IntroductionWindowState& state,
        std::uint32_t window) override {
        Check(window == 0x8800u,
              "first text flow receives window at +0x88");
        calls.push_back('T');
        state.Write(0x98u, 0x9988u);
    }
    void SetPanelTextAndVisibility(
        pal4::IntroductionWindowState&,
        std::uint32_t window) override {
        Check(window == 0x9988u,
              "second text flow rereads window at +0x98");
        calls.push_back('t');
    }
};

pal4::IntroductionWindowState MakeState() {
    pal4::IntroductionWindowState state{};
    state.Write(0x88u, 0x8800u);
    state.Write(0x8cu, 0x8c00u);
    state.Write(0x90u, 0x9000u);
    state.Write(0x94u, 0x9400u);
    state.Write(0x98u, 0x9800u);
    state.Write(0xacu, 0xac00u);
    state.Write(0x3cu, 0x1234u);
    return state;
}

void CheckDirectPanelVisibility() {
    auto state = MakeState();
    struct PanelRuntime final : Runtime {
        void SetVisible(std::uint32_t window, bool visible) override {
            Check(window == 0xac00u, "panel window is at +0xac");
            calls.push_back(visible ? 'v' : 'V');
        }
    } runtime;
    pal4::IntroductionWindowVisibility::SetPanelVisibility(
        state, true, runtime);
    Check(runtime.calls == std::vector<char>{'v'} &&
              runtime.enabled.empty(),
          "showing panel does not enable selector buttons");
    runtime.calls.clear();
    pal4::IntroductionWindowVisibility::SetPanelVisibility(
        state, false, runtime);
    Check(runtime.calls == std::vector<char>{'V', 'E', 'E', 'E', 'E'} &&
              runtime.enabled == std::vector<std::uint32_t>{
                  0x8800u, 0x8c00u, 0x9000u, 0x9400u},
          "hiding panel enables four buttons in address order");
}

void CheckShowAndHide() {
    auto state = MakeState();
    Runtime runtime;
    pal4::IntroductionWindowVisibility::SetVisibility(
        state, 0x1000u, 0x101u, 0x7000u, runtime);
    Check(runtime.parent_showing == 0x101u &&
              runtime.calls == std::vector<char>{
                  'P', 'V', 'E', 'E', 'E', 'E', 'M', 'I', 'T', 't'} &&
              state.Read(0x3cu) == 0x1234u,
          "show uses low byte for branch but passes full flag to dialog");

    runtime.calls.clear();
    pal4::IntroductionWindowVisibility::SetVisibility(
        state, 0x1000u, 0x100u, 0x7000u, runtime);
    Check(runtime.parent_showing == 0x100u &&
              runtime.calls == std::vector<char>{'P', 'M', 'R'} &&
              state.Read(0x3cu) == 0u,
          "hide removes active member and clears +0x3c");
}

}  // namespace

int main() {
    CheckDirectPanelVisibility();
    CheckShowAndHide();
    std::cout << "Introduction window visibility tests passed\n";
}
