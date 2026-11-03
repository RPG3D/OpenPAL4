#include "pal4/actor_control_display.h"

namespace pal4 {

namespace {
std::uint8_t ProjectAndDispatch(std::span<std::uint32_t, 145> words,
                                IActorControlDisplayResetRuntime& runtime) {
    auto position = runtime.ActorPosition(words[127]);
    position[1] += runtime.ActorHeight(words[127]) + 20.0f;
    const auto first_subsystem = runtime.CameraSubsystem();
    const auto second_subsystem = runtime.CameraSubsystem();
    const auto first_projection = runtime.Project(
        runtime.ActiveCamera(second_subsystem), position);
    const auto second_projection = runtime.Project(
        runtime.ActiveCamera(first_subsystem), position);
    const ActorControlScreenPoint mixed{second_projection[0],
                                         first_projection[1]};
    const auto converted = runtime.ConvertCoordinates(
        runtime.UiFrameManager(), second_projection, mixed);
    return runtime.ProcessUiEvents(runtime.UiFrameManager(), 1u,
                                   converted);
}
}  // namespace

ActorControlWorldPoint ActorControlDisplayRuntimeStub::ActorPosition(
    std::uint32_t) { return {}; }
float ActorControlDisplayRuntimeStub::ActorHeight(std::uint32_t) {
    return 0.0f;
}
std::uint32_t ActorControlDisplayRuntimeStub::CameraSubsystem() {
    return 0u;
}
std::uint32_t ActorControlDisplayRuntimeStub::ActiveCamera(
    std::uint32_t) { return 0u; }
ActorControlScreenPoint ActorControlDisplayRuntimeStub::Project(
    std::uint32_t, const ActorControlWorldPoint&) { return {}; }
std::uint32_t ActorControlDisplayRuntimeStub::UiFrameManager() {
    return 0u;
}
void ActorControlDisplayRuntimeStub::ProcessUiState(
    std::uint32_t, std::uint32_t) {}
std::uint32_t ActorControlDisplayRuntimeStub::MonsterManager() {
    return 0u;
}
void ActorControlDisplayRuntimeStub::ClearMonsterFlags(
    std::uint32_t) {}
ActorControlScreenPoint ActorControlDisplayRuntimeStub::ConvertCoordinates(
    std::uint32_t, const ActorControlScreenPoint&,
    const ActorControlScreenPoint&) { return {}; }
std::uint8_t ActorControlDisplayRuntimeStub::ProcessUiEvents(
    std::uint32_t, std::uint32_t,
    const ActorControlScreenPoint&) { return 0u; }
std::uint32_t ActorControlDisplayRuntimeStub::ProgressBar(std::uint32_t) {
    return 0u;
}
void ActorControlDisplayRuntimeStub::SetProgressVisible(
    std::uint32_t, bool) {}
std::uint32_t ActorControlDisplayRuntimeStub::GameArenaManager() {
    return 0u;
}
std::uint8_t ActorControlDisplayRuntimeStub::RemoveClump(
    std::uint32_t, std::uint32_t) { return 0u; }

std::uint8_t ActorControlDatabase::UpdateDisplay(
    ActorControlDatabasePrefix& state, std::uint32_t value,
    IActorControlDisplayRuntime& runtime) {
    return UpdateDisplay(state.words, value, runtime);
}

std::uint8_t ActorControlDatabase::UpdateDisplay(
    std::span<std::uint32_t, 145> words, const std::uint32_t value,
    IActorControlDisplayRuntime& runtime) {
    words[32] = value;
    if (value != 0u || words[28] != 0u) {
        const ActorControlScreenPoint zero{};
        static_cast<void>(runtime.ProcessUiEvents(
            runtime.UiFrameManager(), 0u, zero));
        runtime.SetProgressVisible(
            runtime.ProgressBar(runtime.UiFrameManager()), false);
        return runtime.RemoveClump(runtime.GameArenaManager(), words[40]);
    }

    return ProjectAndDispatch(words, runtime);
}

std::uint8_t ActorControlDatabase::ResetAndUpdateDisplay(
    ActorControlDatabasePrefix& state,
    IActorControlDisplayResetRuntime& runtime) {
    return ResetAndUpdateDisplay(state.words, runtime);
}

std::uint8_t ActorControlDatabase::ResetAndUpdateDisplay(
    std::span<std::uint32_t, 145> words,
    IActorControlDisplayResetRuntime& runtime) {
    words[28] = 0u;
    words[29] = 0u;
    words[30] = 0u;
    runtime.ProcessUiState(runtime.UiFrameManager(), 1u);
    runtime.ClearMonsterFlags(runtime.MonsterManager());
    if (words[127] == 0u) return 0u;
    if (words[32] != 0u) {
        const ActorControlScreenPoint zero{};
        return runtime.ProcessUiEvents(runtime.UiFrameManager(), 0u,
                                       zero);
    }
    return ProjectAndDispatch(words, runtime);
}

}  // namespace pal4
