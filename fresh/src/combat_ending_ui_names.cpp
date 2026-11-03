#include "pal4/combat_ending_ui_names.h"

#include <array>
#include <stdexcept>

namespace pal4 {
namespace {

constexpr std::array<CombatEndingUiNameDescriptor, 9> kDescriptors{{
    {"uiSetupShowStep", 0x8f9578u, 0x8f9580u, 0x544f80u},
    {"uiShowWindowTask", 0x8f9564u, 0x8f956cu, 0x5450e0u},
    {"uiCloseSystemTask", 0x8f9550u, 0x8f9558u, 0x545240u},
    {"uiUpdateExpTask", 0x8f953cu, 0x8f9544u, 0x5453a0u},
    {"uiMoveDownWindowTask", 0x8f9528u, 0x8f9530u, 0x545500u},
    {"uiShowLevelUpHintTask", 0x8f9514u, 0x8f951cu, 0x545660u},
    {"uiShowPrizePropTask", 0x8f9500u, 0x8f9508u, 0x5457c0u},
    {"uiShowScoreText", 0x8f94ecu, 0x8f94f4u, 0x545920u},
    {"uiShowScoreLvText", 0x8f94d8u, 0x8f94e0u, 0x545a80u},
}};

}  // namespace

const CombatEndingUiNameDescriptor& CombatEndingUiNameRegistry::Descriptor(
    const CombatEndingUiNameKind kind) {
    const auto index = static_cast<std::size_t>(kind);
    if (index >= kDescriptors.size()) throw std::out_of_range("UI name kind");
    return kDescriptors[index];
}

int CombatEndingUiNameRegistry::Register(
    const CombatEndingUiNameKind kind, CombatEndingUiNameState& state,
    ICombatEndingUiNameRegistrationRuntime& runtime) {
    InitializeName(kind, state);
    return runtime.RegisterAtExit(Descriptor(kind).cleanup_callback);
}

void CombatEndingUiNameRegistry::InitializeName(
    const CombatEndingUiNameKind kind, CombatEndingUiNameState& state) {
    // The original initializers read an indeterminate stack byte here.
    state.text_flag = 0u;
    state.name = Descriptor(kind).name;
}

void CombatEndingUiNameRegistry::Cleanup(
    const CombatEndingUiNameKind kind, CombatEndingUiNameState& state) {
    (void)Descriptor(kind);
    if ((state.cleanup_guard & 1u) != 0u) return;
    state.cleanup_guard |= 1u;
    std::string{}.swap(state.name);
}

int CombatEndingShowStepRegistration::Register(
    CombatEndingUiNameState& state,
    ICombatEndingUiNameRegistrationRuntime& runtime) {
    return CombatEndingUiNameRegistry::Register(
        CombatEndingUiNameKind::ShowStep, state, runtime);
}

void CombatEndingShowStepRegistration::InitializeName(
    CombatEndingUiNameState& state) {
    CombatEndingUiNameRegistry::InitializeName(
        CombatEndingUiNameKind::ShowStep, state);
}

void CombatEndingShowStepRegistration::Cleanup(
    CombatEndingUiNameState& state) {
    CombatEndingUiNameRegistry::Cleanup(
        CombatEndingUiNameKind::ShowStep, state);
}

int CombatEndingShowWindowTaskRegistration::Register(
    CombatEndingUiNameState& state,
    ICombatEndingUiNameRegistrationRuntime& runtime) {
    return CombatEndingUiNameRegistry::Register(
        CombatEndingUiNameKind::ShowWindowTask, state, runtime);
}

void CombatEndingShowWindowTaskRegistration::InitializeName(
    CombatEndingUiNameState& state) {
    CombatEndingUiNameRegistry::InitializeName(
        CombatEndingUiNameKind::ShowWindowTask, state);
}

void CombatEndingShowWindowTaskRegistration::Cleanup(
    CombatEndingUiNameState& state) {
    CombatEndingUiNameRegistry::Cleanup(
        CombatEndingUiNameKind::ShowWindowTask, state);
}

}  // namespace pal4
