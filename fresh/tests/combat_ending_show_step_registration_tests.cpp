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
        ++calls;
        callback = callback_address;
        Check(state && state->name == "uiSetupShowStep" &&
                  state->text_flag == 0u,
              "name initialized before atexit registration");
        return -7;
    }
};

void RegistrationAndCleanup() {
    pal4::CombatEndingUiNameState state;
    state.name = "prior";
    state.text_flag = 0xffu;
    Runtime runtime;
    runtime.state = &state;
    Check(pal4::CombatEndingShowStepRegistration::Register(
              state, runtime) == -7,
          "atexit return forwarded");
    Check(runtime.calls == 1u && runtime.callback == 0x544f80u &&
              state.name == "uiSetupShowStep",
          "show-step registration identity");
    state.cleanup_guard = 0xa4u;
    pal4::CombatEndingShowStepRegistration::Cleanup(state);
    Check(state.name.empty() && state.cleanup_guard == 0xa5u,
          "cleanup clears name and preserves unrelated guard bits");
    state.name = "new value";
    pal4::CombatEndingShowStepRegistration::Cleanup(state);
    Check(state.name == "new value" && state.cleanup_guard == 0xa5u,
          "second cleanup is ignored");
}

void IndependentInitializer() {
    pal4::CombatEndingUiNameState state;
    state.cleanup_guard = 1u;
    state.name = "old";
    state.text_flag = 0xabu;
    pal4::CombatEndingShowStepRegistration::InitializeName(state);
    Check(state.name == "uiSetupShowStep" &&
              state.text_flag == 0u && state.cleanup_guard == 1u,
          "name initializer leaves separate cleanup guard alone");
}

}  // namespace

int main() {
    try {
        RegistrationAndCleanup();
        IndependentInitializer();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
