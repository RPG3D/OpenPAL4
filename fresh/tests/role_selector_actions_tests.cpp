#include "pal4/role_selector_actions.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::IRoleSelectorActionRuntime {
    std::unordered_map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t invoked_callback = 0u;
    std::array<std::uint32_t, 2> invoked_arguments{};
    std::uint32_t layout_object = 0u;
    std::uint32_t closed_object = 0u;
    std::uint32_t closed_visible = 1u;
    std::uint32_t closed_parent = 1u;
    std::uint32_t key_navigation = 0u;
    std::uint32_t key_event = 0u;
    std::uint32_t refreshed_navigation = 0u;

    std::uint32_t ReadWord(std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    void InvokeChoice(
        std::uint32_t callback,
        const std::array<std::uint32_t, 2>& arguments) override {
        calls.emplace_back("invoke");
        invoked_callback = callback;
        invoked_arguments = arguments;
    }
    void LayoutButtons(std::uint32_t object) override {
        calls.emplace_back("layout");
        layout_object = object;
    }
    void Close(std::uint32_t object,
               std::uint32_t visible,
               std::uint32_t parent) override {
        calls.emplace_back("close");
        closed_object = object;
        closed_visible = visible;
        closed_parent = parent;
    }
    void ForwardKeyEvent(std::uint32_t navigation,
                         std::uint32_t event) override {
        calls.emplace_back("key");
        key_navigation = navigation;
        key_event = event;
    }
    void RefreshNavigation(std::uint32_t navigation) override {
        calls.emplace_back("refresh");
        refreshed_navigation = navigation;
    }
};

void CheckRoleClick(std::int32_t index, bool callback_present) {
    Runtime runtime;
    runtime.words[0x2008u] = 0x3000u;
    runtime.words[0x3118u] = static_cast<std::uint32_t>(index);
    runtime.words[0x1040u] = callback_present ? 0x4000u : 0u;
    runtime.words[0x1044u] = 0x55u;
    runtime.words[0x4000u] = 0x9000u;
    if (index >= 0 && index < 4)
        runtime.words[0x1078u +
                      static_cast<std::uint32_t>(index) * 52u] =
            static_cast<std::uint32_t>(index) + 10u;
    Check(pal4::RoleSelectorActions::HandleRoleClick(
              0x1000u, 0x2000u, runtime) == 1u,
          "click returns one");
    if (index >= 4) {
        Check(runtime.calls ==
                  std::vector<std::string>{"read", "read"},
              "index four or above does nothing");
        return;
    }
    Check(runtime.layout_object == 0x1000u &&
          runtime.calls.back() == "layout",
          "signed index below four relayouts");
    if (!callback_present) {
        Check(runtime.invoked_callback == 0u &&
              runtime.calls == std::vector<std::string>{
                  "read", "read", "read", "layout"},
              "missing callback skips invocation");
        return;
    }
    Check(runtime.invoked_callback == 0x9000u &&
          runtime.invoked_arguments ==
              (std::array<std::uint32_t, 2>{
                  index < 0 ? 0x55u :
                      static_cast<std::uint32_t>(index) + 10u,
                  0x55u}) &&
          runtime.calls[runtime.calls.size() - 2u] == "invoke",
          "clicked role and context forwarded before layout");
}

void CheckClose() {
    Runtime runtime;
    Check(pal4::RoleSelectorActions::HandleCloseClick(
              0x1000u, runtime) == 1u &&
          runtime.closed_object == 0x1000u &&
          runtime.closed_visible == 0u &&
          runtime.closed_parent == 0u &&
          runtime.calls == std::vector<std::string>{"close"},
          "close button invokes virtual slot with two zeros");
}

void CheckKeyEvent() {
    Runtime runtime;
    Check(pal4::RoleSelectorActions::HandleKeyEvent(
              0x1000u, 0x2000u, runtime) == 1u &&
          runtime.key_navigation == 0x1028u &&
          runtime.key_event == 0x2000u &&
          runtime.refreshed_navigation == 0x1028u &&
          runtime.calls == std::vector<std::string>{"key", "refresh"},
          "key event is forwarded then navigation is refreshed");
}

}  // namespace

int main() {
    try {
        for (std::int32_t index = 0; index < 4; ++index) {
            CheckRoleClick(index, false);
            CheckRoleClick(index, true);
        }
        CheckRoleClick(-1, true);
        CheckRoleClick(4, true);
        CheckRoleClick(5, false);
        CheckClose();
        CheckKeyEvent();
        std::cout << "role_selector_actions_tests passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
