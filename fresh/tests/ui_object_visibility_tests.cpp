#include "pal4/ui_object_visibility.h"

#include <cstdint>
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

struct Runtime final : pal4::UiObjectVisibilityRuntimeStub {
    std::vector<char> events;
    std::vector<std::uint32_t> enabled;
    void SetParent(pal4::UiObjectState&,
                   std::uint8_t showing, std::uint32_t parent) override {
        Check((showing == 0u || showing == 1u) && parent == 0x6000u,
              "parent setter receives caller flag and parent window");
        events.push_back('P');
    }
    void SetVisible(std::uint32_t window, bool visible) override {
        Check(window == 0x5000u, "visibility targets +0x50 window");
        events.push_back(visible ? 'V' : 'v');
    }
    void SetEnabled(std::uint32_t window, bool value) override {
        Check(value, "false visibility enables both auxiliary windows");
        enabled.push_back(window);
        events.push_back('E');
    }
    std::uint32_t FrameManager() override {
        events.push_back('M');
        return 0x9000u;
    }
    void InsertActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x9134u && member == 0x7028u,
              "insert uses manager +0x134 and object +0x28");
        events.push_back('I');
    }
    int RemoveActive(std::uint32_t collection,
                      std::uint32_t member) override {
        Check(collection == 0x9134u && member == 0x7028u,
              "remove uses manager +0x134 and object +0x28");
        events.push_back('R');
        return -31;
    }
    int SetTextAndVisibility(pal4::UiObjectState&,
                             std::uint32_t window) override {
        Check(window == 0x4800u,
              "show branch forwards +0x48 window");
        events.push_back('T');
        return 27;
    }
};

pal4::UiObjectState MakeState() {
    pal4::UiObjectState state{};
    state.Write(0x48u, 0x4800u);
    state.Write(0x4cu, 0x4c00u);
    state.Write(0x50u, 0x5000u);
    state.Write(0x3cu, 0x1234u);
    return state;
}

void CheckDirectWindowVisibility() {
    auto state = MakeState();
    Runtime runtime;
    pal4::UiObjectVisibility::SetWindowVisibility(state, true, runtime);
    Check(runtime.events == std::vector<char>{'V'} &&
              runtime.enabled.empty(),
          "visible branch does not touch enabled state");
    runtime.events.clear();
    pal4::UiObjectVisibility::SetWindowVisibility(state, false, runtime);
    Check(runtime.events == std::vector<char>{'v', 'E', 'E'} &&
              runtime.enabled ==
                  std::vector<std::uint32_t>{0x4800u, 0x4c00u},
          "hidden branch enables two auxiliary windows in order");
}

void CheckShowAndHide() {
    auto state = MakeState();
    Runtime runtime;
    Check(pal4::UiObjectVisibility::SetVisibility(
              state, 0x7000u, 1u, 0x6000u, runtime) == 27 &&
              runtime.events ==
                  std::vector<char>{'P', 'v', 'E', 'E', 'M', 'I', 'T'} &&
              state.Read(0x3cu) == 0x1234u,
          "show sets parent and window state before inserting member");
    runtime.events.clear();
    runtime.enabled.clear();
    Check(pal4::UiObjectVisibility::SetVisibility(
              state, 0x7000u, 0u, 0x6000u, runtime) == -31 &&
              runtime.events ==
                  std::vector<char>{'P', 'v', 'E', 'E', 'M', 'R'} &&
              state.Read(0x3cu) == 0u,
          "hide removes member then clears +0x3c");
}

}  // namespace

int main() {
    CheckDirectWindowVisibility();
    CheckShowAndHide();
    std::cout << "UI object visibility tests passed\n";
}
