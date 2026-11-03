#include "pal4/combat_ending_ui_names.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Runtime final : pal4::ICombatEndingUiNameRegistrationRuntime {
    pal4::CombatEndingUiNameState* state = nullptr;
    std::uint32_t callback = 0u;
    std::uint32_t calls = 0u;

    int RegisterAtExit(std::uint32_t callback_address) override {
        Check(state && state->name == "uiShowWindowTask" &&
                  state->text_flag == 0u,
              "show-window name set before CRT registration");
        ++calls;
        callback = callback_address;
        return 0x45;
    }
};

void RegisterAndCleanup() {
    pal4::CombatEndingUiNameState show_window;
    pal4::CombatEndingUiNameState show_step;
    show_window.name = "old";
    show_window.text_flag = 0xffu;
    show_step.name = "uiSetupShowStep";
    Runtime runtime;
    runtime.state = &show_window;

    Check(pal4::CombatEndingShowWindowTaskRegistration::Register(
              show_window, runtime) == 0x45,
          "registration result forwarded");
    Check(runtime.calls == 1u && runtime.callback == 0x5450e0u &&
              show_window.name == "uiShowWindowTask" &&
              show_step.name == "uiSetupShowStep",
          "registration callback and independent names");

    show_window.cleanup_guard = 0x80u;
    pal4::CombatEndingShowWindowTaskRegistration::Cleanup(show_window);
    Check(show_window.name.empty() && show_window.cleanup_guard == 0x81u &&
              show_step.name == "uiSetupShowStep",
          "one-time cleanup keeps the other name intact");
    show_window.name = "later";
    pal4::CombatEndingShowWindowTaskRegistration::Cleanup(show_window);
    Check(show_window.name == "later", "cleanup guard blocks repeat");
}

void StandaloneInitializer() {
    pal4::CombatEndingUiNameState state;
    state.cleanup_guard = 1u;
    state.text_flag = 0xabu;
    pal4::CombatEndingShowWindowTaskRegistration::InitializeName(state);
    Check(state.name == "uiShowWindowTask" &&
              state.text_flag == 0u && state.cleanup_guard == 1u,
          "initializer defines flag without resetting cleanup guard");
}

}  // namespace

int main() {
    try {
        RegisterAndCleanup();
        StandaloneInitializer();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
