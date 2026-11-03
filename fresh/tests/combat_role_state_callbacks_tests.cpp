#include "pal4/combat_role_state_callbacks.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(const bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatRoleStateCallbackRuntime {
    std::map<std::uint32_t, std::uint32_t> words;
    std::vector<std::string> calls;
    std::uint32_t ui_tree = 0u;
    std::uint32_t ui_key = 0u;
    std::array<std::uint32_t, 2> ui_payload{};
    std::uint32_t script_router = 0u;
    std::uint32_t script_key = 0u;
    std::uint32_t script_facet = 0u;
    std::uint32_t script_mode = 0u;

    std::uint32_t ReadWord(const std::uint32_t address) override {
        calls.emplace_back("read");
        return words.at(address);
    }
    std::uint32_t UiFrameManager() override {
        calls.emplace_back("ui_manager");
        return 0x3000u;
    }
    void DispatchUiEvent(
        const std::uint32_t tree, const std::uint32_t key,
        const std::array<std::uint32_t, 2>& payload) override {
        calls.emplace_back("ui_event");
        ui_tree = tree;
        ui_key = key;
        ui_payload = payload;
    }
    std::uint32_t ScriptEventRouter() override {
        calls.emplace_back("script_router");
        return 0x7000u;
    }
    void DispatchScriptEvent(const std::uint32_t router,
                             const std::uint32_t key,
                             const std::uint32_t facet,
                             const std::uint32_t mode) override {
        calls.emplace_back("script_event");
        script_router = router;
        script_key = key;
        script_facet = facet;
        script_mode = mode;
    }
};

void MouseClick() {
    Runtime runtime;
    Check(pal4::CombatRoleStateCallbacks::OnMouseClick(0x4400u, runtime) ==
              1u &&
          runtime.ui_tree == 0x3000u + 260u &&
          runtime.ui_key ==
              pal4::CombatRoleStateCallbacks::kUiEventKeyAddress &&
          runtime.ui_payload == std::array<std::uint32_t, 2>{0u, 0u} &&
          runtime.calls ==
              std::vector<std::string>{"ui_manager", "ui_event"},
          "mouse callback dispatches zero payload to the fixed UI key");
}

void SubstitutionClick() {
    Runtime runtime;
    constexpr std::uint32_t event = 0x4000u;
    constexpr std::uint32_t control = 0x5000u;
    constexpr std::uint32_t actor = 0x6000u;
    runtime.words[event + 8u] = control;
    runtime.words[control + 280u] = 0u;
    Check(pal4::CombatRoleStateCallbacks::OnSubstitutionClick(
              event, runtime) == 0u &&
          runtime.calls == std::vector<std::string>{"read", "read"},
          "empty button actor skips router lookup and event dispatch");

    runtime.calls.clear();
    runtime.words[control + 280u] = actor;
    runtime.words[pal4::CombatRoleStateCallbacks::kSubstituteKeyAddress] =
        0x8100u;
    Check(pal4::CombatRoleStateCallbacks::OnSubstitutionClick(
              event, runtime) == 1u &&
          runtime.script_router == 0x7000u &&
          runtime.script_key == 0x8100u &&
          runtime.script_facet == actor + 716u &&
          runtime.script_mode == 1u &&
          runtime.calls == std::vector<std::string>{
              "read", "read", "read", "script_router", "script_event"},
          "substitute event passes the actor facet at +716 to the script router");
}

}  // namespace

int main() {
    try {
        MouseClick();
        SubstitutionClick();
        std::cout << "combat_role_state_callbacks_tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
