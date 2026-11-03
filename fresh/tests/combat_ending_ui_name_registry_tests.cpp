#include "pal4/combat_ending_ui_names.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void Check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

struct Expected {
    pal4::CombatEndingUiNameKind kind;
    std::string_view name;
    std::uint32_t guard;
    std::uint32_t text;
    std::uint32_t cleanup;
};

constexpr std::array kExpected{
    Expected{pal4::CombatEndingUiNameKind::ShowStep,
             "uiSetupShowStep", 0x8f9578u, 0x8f9580u, 0x544f80u},
    Expected{pal4::CombatEndingUiNameKind::ShowWindowTask,
             "uiShowWindowTask", 0x8f9564u, 0x8f956cu, 0x5450e0u},
    Expected{pal4::CombatEndingUiNameKind::CloseSystemTask,
             "uiCloseSystemTask", 0x8f9550u, 0x8f9558u, 0x545240u},
    Expected{pal4::CombatEndingUiNameKind::UpdateExpTask,
             "uiUpdateExpTask", 0x8f953cu, 0x8f9544u, 0x5453a0u},
    Expected{pal4::CombatEndingUiNameKind::MoveDownWindowTask,
             "uiMoveDownWindowTask", 0x8f9528u, 0x8f9530u, 0x545500u},
    Expected{pal4::CombatEndingUiNameKind::ShowLevelUpHintTask,
             "uiShowLevelUpHintTask", 0x8f9514u, 0x8f951cu, 0x545660u},
    Expected{pal4::CombatEndingUiNameKind::ShowPrizePropTask,
             "uiShowPrizePropTask", 0x8f9500u, 0x8f9508u, 0x5457c0u},
    Expected{pal4::CombatEndingUiNameKind::ShowScoreText,
             "uiShowScoreText", 0x8f94ecu, 0x8f94f4u, 0x545920u},
    Expected{pal4::CombatEndingUiNameKind::ShowScoreLvText,
             "uiShowScoreLvText", 0x8f94d8u, 0x8f94e0u, 0x545a80u},
};

struct Runtime final : pal4::ICombatEndingUiNameRegistrationRuntime {
    pal4::CombatEndingUiNameState* state = nullptr;
    std::string_view expected_name;
    std::uint32_t expected_cleanup = 0u;
    std::uint32_t calls = 0u;

    int RegisterAtExit(std::uint32_t callback) override {
        Check(state && state->name == expected_name &&
                  state->text_flag == 0u &&
                  callback == expected_cleanup,
              "descriptor name initialized before matching CRT callback");
        ++calls;
        return 13;
    }
};

void AllNames() {
    for (const auto& expected : kExpected) {
        const auto& descriptor =
            pal4::CombatEndingUiNameRegistry::Descriptor(expected.kind);
        Check(descriptor.name == expected.name &&
                  descriptor.guard_address == expected.guard &&
                  descriptor.text_address == expected.text &&
                  descriptor.cleanup_callback == expected.cleanup,
              "IDA-backed UI name descriptor");

        pal4::CombatEndingUiNameState state;
        state.text_flag = 0xadu;
        state.cleanup_guard = 0x80u;
        Runtime runtime;
        runtime.state = &state;
        runtime.expected_name = expected.name;
        runtime.expected_cleanup = expected.cleanup;
        Check(pal4::CombatEndingUiNameRegistry::Register(
                  expected.kind, state, runtime) == 13 &&
                  runtime.calls == 1u && state.cleanup_guard == 0x80u,
              "registration forwards result without changing cleanup guard");

        pal4::CombatEndingUiNameRegistry::Cleanup(expected.kind, state);
        Check(state.name.empty() && state.cleanup_guard == 0x81u,
              "first cleanup releases only this name");
        state.name = "retained";
        pal4::CombatEndingUiNameRegistry::Cleanup(expected.kind, state);
        Check(state.name == "retained", "repeat cleanup leaves state alone");

        state.text_flag = 0xffu;
        pal4::CombatEndingUiNameRegistry::InitializeName(expected.kind, state);
        Check(state.name == expected.name && state.text_flag == 0u &&
                  state.cleanup_guard == 0x81u,
              "initializer replaces name without clearing cleanup guard");
    }
}

void InvalidKind() {
    bool threw = false;
    try {
        (void)pal4::CombatEndingUiNameRegistry::Descriptor(
            static_cast<pal4::CombatEndingUiNameKind>(99));
    } catch (const std::out_of_range&) {
        threw = true;
    }
    Check(threw, "invalid registry index rejected");
}

}  // namespace

int main() {
    try {
        AllNames();
        InvalidKind();
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
