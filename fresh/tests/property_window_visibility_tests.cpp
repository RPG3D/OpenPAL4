#include "pal4/property_window_visibility.h"

#include <cstdlib>
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

struct Runtime final : pal4::IPropertyWindowVisibilityRuntime {
    std::vector<std::string> calls;

    void SetParent(pal4::PropertyWindowState&,
                   std::uint8_t visible,
                   std::uint32_t parent) override {
        Check(parent == 0x7000u, "parent is forwarded");
        calls.push_back("parent:" + std::to_string(visible));
    }
    void SetItemPanel(pal4::PropertyWindowState&,
                      bool visible) override {
        Check(!visible, "item panel is hidden first");
        calls.emplace_back("hide-panel");
    }
    void Broadcast(std::uint32_t event, std::uint32_t first,
                   std::uint32_t second) override {
        calls.push_back("event:" + std::to_string(event) + ":" +
                        std::to_string(first) + ":" +
                        std::to_string(second));
    }
    void SetSelected(std::uint32_t button,
                     bool selected) override {
        Check(!selected, "category buttons are cleared");
        calls.push_back("select:" + std::to_string(button));
    }
    void AddActiveMember(pal4::PropertyWindowState&,
                         std::uint32_t offset) override {
        Check(offset == 40u, "active member uses +0x28");
        calls.emplace_back("add-member");
    }
    void RemoveActiveMember(pal4::PropertyWindowState&,
                            std::uint32_t offset) override {
        Check(offset == 40u, "removed member uses +0x28");
        calls.emplace_back("remove-member");
    }
    void ToggleModel(pal4::PropertyWindowState&,
                     bool visible) override {
        Check(!visible, "close hides property model");
        calls.emplace_back("hide-model");
    }
    void UpdatePropertyPanel(
        pal4::PropertyWindowState& state) override {
        Check(state.Read(108u) == 5u,
              "category five is set before panel refresh");
        calls.emplace_back("update-panel");
    }
    void UpdateSystemDialog(std::uint32_t key) override {
        Check(key == 2u, "opening updates system dialog key two");
        calls.emplace_back("system-dialog");
    }
};

void CheckVisible(std::uint8_t visible) {
    pal4::PropertyWindowState state;
    state.Write(36u, 0xabcdu);
    for (std::uint32_t offset = 64u; offset <= 84u; offset += 4u) {
        state.Write(offset, 0x2000u + offset);
    }
    Runtime runtime;
    pal4::PropertyWindowVisibility::Set(state, visible, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent:" + std::to_string(visible),
              "hide-panel",
              "event:9114516:" + std::to_string(visible) + ":0",
              "event:9114528:" + std::to_string(visible) + ":43981",
              "select:8256", "select:8260", "select:8264",
              "select:8268", "select:8272", "select:8276",
              "event:9114532:6:0", "add-member", "update-panel",
              "system-dialog"} && state.Read(108u) == 5u,
          "opening broadcasts, clears buttons and refreshes property panel");
}

void CheckHidden() {
    pal4::PropertyWindowState state;
    state.Write(36u, 0xabcdu);
    for (std::uint32_t offset = 64u; offset <= 84u; offset += 4u) {
        state.Write(offset, 0x2000u + offset);
    }
    Runtime runtime;
    pal4::PropertyWindowVisibility::Set(state, 0u, 0x7000u, runtime);
    Check(runtime.calls == std::vector<std::string>{
              "parent:0", "hide-panel", "event:9114516:0:0",
              "event:9114528:0:43981", "select:8256", "select:8260",
              "select:8264", "select:8268", "select:8272", "select:8276",
              "hide-model", "remove-member"},
          "closing hides model and removes active member after common steps");
}

}  // namespace

int main() {
    CheckVisible(1u);
    CheckVisible(2u);
    CheckHidden();
    std::cout << "Property window visibility tests passed\n";
}
