#include "pal4/script_player_blend.h"

namespace pal4 {

std::uint32_t ScriptPlayerBlend::BlendOut(
    std::int32_t index, float duration, std::uint8_t wait_mode) {
    return Start(index, duration, wait_mode, false);
}

std::uint32_t ScriptPlayerBlend::BlendIn(
    std::int32_t index, float duration, std::uint8_t wait_mode) {
    return Start(index, duration, wait_mode, true);
}

std::uint32_t ScriptPlayerBlend::Start(
    std::int32_t index, float duration,
    std::uint8_t wait_mode, bool in) {
    const auto manager = runtime_.PlayerManager();
    const auto actor = runtime_.PlayerByIndex(manager, index);
    return StartActor(actor, duration, wait_mode, in, false);
}

std::uint32_t ScriptPlayerBlend::BlendNpcOut(
    std::string_view name, float duration, std::uint8_t wait_mode) {
    const auto manager = runtime_.NpcManager();
    return StartActor(runtime_.NpcByName(manager, name), duration,
                      wait_mode, false, true);
}

std::uint32_t ScriptPlayerBlend::BlendNpcIn(
    std::string_view name, float duration, std::uint8_t wait_mode) {
    const auto manager = runtime_.NpcManager();
    return StartActor(runtime_.NpcByName(manager, name), duration,
                      wait_mode, true, true);
}

std::uint32_t ScriptPlayerBlend::StartActor(
    std::uint32_t actor, float duration, std::uint8_t wait_mode,
    bool in, bool npc) {
    if (actor == 0u) return 0u;
    auto* state = runtime_.Allocate(sizeof(PlayerBlendWaitState));
    if (state != nullptr) {
        runtime_.Construct(*state, wait_mode,
                           runtime_.ScriptExecutor() + kQueueOffset);
        state->actor = actor;
        state->duration = duration;
        state->elapsed = 0.0f;
        state->vtable_address = npc ?
            (in ? kNpcInVtable : kNpcOutVtable) :
            (in ? kInVtable : kOutVtable);
        runtime_.Flush(*state);
        if (in) {
            std::uint32_t info = 0u;
            if (npc) {
                const auto name = runtime_.ActorName(actor);
                info = runtime_.NpcInfoByName(runtime_.NpcManager(), name);
            }
            runtime_.RemoveFromArena(runtime_.ArenaManager(), actor);
            runtime_.SetStateFlag(actor, 1u);
            if (info != 0u) runtime_.ProcessNpcEvents(actor, info, 0u);
            runtime_.AddToArena(runtime_.ArenaManager(), actor);
            runtime_.SetMaterialFlags(actor, 0x00ffffffu);
        } else {
            runtime_.SetMaterialFlags(actor, 0xffffffffu);
        }
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptPlayerBlend::UpdateOut(
    PlayerBlendWaitState& state, double frame_time) {
    const auto elapsed = state.elapsed + frame_time;
    state.elapsed = static_cast<float>(elapsed);
    if (elapsed < state.duration) {
        const auto fraction = static_cast<float>(elapsed / state.duration);
        const auto value = runtime_.Interpolate(255.0f, 0.0f,
                                                fraction, 0u);
        const auto alpha = runtime_.ConvertAlpha(value);
        const auto flags = 0x00ffffffu |
            (static_cast<std::uint32_t>(alpha & 0xff) << 24u);
        runtime_.SetMaterialFlags(state.actor, flags);
    } else {
        state.elapsed = state.duration;
        FinishOut(state);
    }
}

void ScriptPlayerBlend::FinishOut(PlayerBlendWaitState& state) {
    runtime_.SetMaterialFlags(state.actor, 0xffffffffu);
    runtime_.RemoveFromArena(runtime_.ArenaManager(), state.actor);
    runtime_.Close(state);
}

void ScriptPlayerBlend::UpdateIn(PlayerBlendWaitState& state, double frame_time) {
    const auto elapsed = state.elapsed + frame_time;
    state.elapsed = static_cast<float>(elapsed);
    if (elapsed < state.duration) {
        const auto fraction = static_cast<float>(elapsed / state.duration);
        const auto alpha = runtime_.ConvertAlpha(
            runtime_.Interpolate(0.0f, 255.0f, fraction, 0u));
        runtime_.SetMaterialFlags(state.actor, 0x00ffffffu |
            (static_cast<std::uint32_t>(alpha & 0xff) << 24u));
    } else {
        state.elapsed = state.duration;
        Cleanup(state);
    }
}

void ScriptPlayerBlend::Cleanup(PlayerBlendWaitState& state) {
    runtime_.SetMaterialFlags(state.actor, 0xffffffffu);
    runtime_.Close(state);
}

}  // namespace pal4
