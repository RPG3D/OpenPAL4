#include "pal4/pal_actor_control_status_mode.h"

namespace pal4 {

std::uint32_t IActorControlStatusModeRuntime::UiFrameManager() { return 0; }
std::uint32_t IActorControlStatusModeRuntime::FindFrame(
    std::uint32_t, std::string_view) { return 0; }
bool IActorControlStatusModeRuntime::IsFrameVisible(std::uint32_t) {
    return false;
}
void IActorControlStatusModeRuntime::ProcessUiEvent(
    std::uint32_t, std::uint32_t,
    const std::array<std::uint32_t, 2>&) {}
void IActorControlStatusModeRuntime::StartPlayerStatus(
    PalActorControlState&, float, float, float) {}
void IActorControlStatusModeRuntime::SetAnimationFlag(
    std::uint32_t, std::uint32_t) {}
std::uint32_t IActorControlStatusModeRuntime::MonsterManager() { return 0; }
void IActorControlStatusModeRuntime::ClearMonsterFlags(std::uint32_t) {}
std::uint32_t IActorControlStatusModeRuntime::ProcessMonsters(
    std::uint32_t, long double) { return 0; }
bool IActorControlStatusModeRuntime::DialogNeedsUpdate(std::uint32_t*) {
    return false;
}
void IActorControlStatusModeRuntime::UpdateDialog(std::uint32_t*) {}
std::uint32_t IActorControlStatusModeRuntime::PlayerManager() { return 0; }
std::uint32_t IActorControlStatusModeRuntime::FindPlayer(
    std::uint32_t, std::uint32_t) { return 0; }
void IActorControlStatusModeRuntime::ClearStatusHandles(
    PalActorControlState&) {}
bool IActorControlStatusModeRuntime::PositionInRange(std::uint32_t) {
    return false;
}
void IActorControlStatusModeRuntime::BeginMovementStatus(
    PalActorControlState&, float) {}
void IActorControlStatusModeRuntime::ApplyStatusIfPlayerExists(
    PalActorControlState&, float) {}
ActorControlEffectCreation IActorControlStatusModeRuntime::CreateEffect(
    std::string_view, std::uint32_t, std::uint32_t,
    std::uint32_t) { return {}; }
void IActorControlStatusModeRuntime::ResetTemporary(
    ActorControlEffectHandle& temporary) {
    temporary = {0xffffffffu, 0u};
}
std::uint32_t IActorControlStatusModeRuntime::AudioManager() { return 0; }
std::uint32_t IActorControlStatusModeRuntime::PlayAudio(
    std::uint32_t, const std::uint32_t*, std::string_view,
    std::uint32_t) { return 0; }
void IActorControlStatusModeRuntime::PlayAudioScratch(
    std::uint32_t, std::string_view, std::uint32_t) {}

namespace {

void RefreshHandle(PalActorControlState& state,
                   IActorControlStatusModeRuntime& runtime) {
    auto* handle = state.words.data() + 203u;
    if (runtime.DialogNeedsUpdate(handle)) {
        runtime.UpdateDialog(handle);
    }
}

void CreateStatusEffect(PalActorControlState& state,
                        IActorControlStatusModeRuntime& runtime,
                        const std::string_view name,
                        const std::uint32_t first_flag) {
    auto created = runtime.CreateEffect(
        name, state.words[1u] + 0xacu, first_flag, 0u);
    state.words[203u] = created.handle.object;
    state.words[204u] = created.handle.token;
    runtime.ResetTemporary(created.handle);
}

void ClearActorAndMonsters(PalActorControlState& state,
                           IActorControlStatusModeRuntime& runtime) {
    runtime.SetAnimationFlag(state.words[1u] + 0x170u, 0x3f800000u);
    runtime.ClearMonsterFlags(runtime.MonsterManager());
}

void PlayStatusAudio(IActorControlStatusModeRuntime& runtime,
                     const std::string_view path) {
    runtime.PlayAudioScratch(runtime.AudioManager(), path, 1u);
}

}  // namespace

std::uint32_t PalActorControlStatusMode::Activate(
    PalActorControlState& state, const long double monster_context,
    const std::uint32_t mode, IActorControlStatusModeRuntime& runtime) {
    const auto actor = state.words[1u];
    if (actor == 0u) {
        return 0u;
    }
    state.words[202u] = mode;
    if (mode < 1u || mode > 3u) {
        return mode - 3u;
    }

    const std::array<std::uint32_t, 2> payload{1u, 0u};
    runtime.ProcessUiEvent(runtime.UiFrameManager() + 0x104u,
                           mode == 1u ? 0x8b1204u : 0x8b1320u, payload);
    if (mode == 1u) {
        runtime.StartPlayerStatus(state, 30.0f, 2.0f, 7.0f);
    }
    runtime.SetAnimationFlag(actor + 0x170u, 0u);
    const auto monster_result = runtime.ProcessMonsters(
        runtime.MonsterManager(), monster_context);
    if (mode != 1u) {
        return monster_result;
    }

    auto* handle = state.words.data() + 203u;
    if (runtime.DialogNeedsUpdate(handle)) {
        runtime.UpdateDialog(handle);
    }
    auto created = runtime.CreateEffect(
        "H_212", actor + 0xacu, 0xffffffffu, 0u);
    handle[0] = created.handle.object;
    handle[1] = created.handle.token;
    runtime.ResetTemporary(created.handle);
    return runtime.PlayAudio(runtime.AudioManager(), &mode,
                             "gamedata\\PALSound\\WA30.wav", 1u);
}

std::uint32_t PalActorControlStatusMode::Poll(
    PalActorControlState& state, IActorControlStatusModeRuntime& runtime) {
    const auto mode = state.words[202u];
    if (mode < 1u || mode > 3u) {
        return 0u;
    }
    const auto frame = runtime.FindFrame(
        runtime.UiFrameManager(),
        mode == 1u ? "ClickGameWindow" : "RepeatGameWindow");
    if (frame == 0u) {
        return 0u;
    }

    if (mode == 1u) {
        const auto player = runtime.FindPlayer(
            runtime.PlayerManager(), state.words[19u]);
        if (player == 0u) {
            ClearActorAndMonsters(state, runtime);
            runtime.ClearStatusHandles(state);
            return 0u;
        }
    }
    if (runtime.IsFrameVisible(frame)) {
        return 1u;
    }

    if (mode != 1u) {
        state.words[202u] = 0u;
    }
    ClearActorAndMonsters(state, runtime);
    if (mode == 1u) {
        runtime.ClearStatusHandles(state);
        PlayStatusAudio(runtime, "gamedata\\PALSound\\WA31.wav");
        return 0u;
    }

    RefreshHandle(state, runtime);
    const bool in_range = runtime.PositionInRange(frame);
    if (in_range) {
        PlayStatusAudio(runtime, "gamedata\\PALSound\\WA32.wav");
        return 0u;
    }
    if (mode == 2u) {
        runtime.ApplyStatusIfPlayerExists(state, 25.0f);
        RefreshHandle(state, runtime);
        CreateStatusEffect(state, runtime, "H_188", 1u);
        return 0u;
    }

    runtime.BeginMovementStatus(state, 20.0f);
    RefreshHandle(state, runtime);
    CreateStatusEffect(state, runtime, "H_189", 0xffffffffu);
    PlayStatusAudio(runtime, "gamedata\\PALSound\\WA29.wav");
    return 0u;
}

}  // namespace pal4
