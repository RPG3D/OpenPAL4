#include "pal4/script_npc_fly_move.h"

#include <string>

namespace pal4 {

std::uint32_t ScriptNpcFlyMove::Create(
    std::string_view npc_name, const NpcFlyVector& target,
    std::uint8_t wait_mode) {
    const auto npc = runtime_.FindNpc(runtime_.NpcManager(), npc_name);
    if (npc == 0u) return 0u;

    runtime_.SetAnimation(npc, "C02");
    runtime_.SetAnimationFlag(npc, 1u);
    const auto name = std::string(npc_name) + "-Npc-Rotate";
    auto* state = runtime_.AllocateWait(sizeof(NpcFlyWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->phase = 2u;
        state->vtable_address = kWaitVtable;
        runtime_.AssignName(*state, name);
        state->npc = npc;
        state->start = runtime_.ReadPosition(npc);
        state->speed = kSpeed;
        state->target = target;
        NpcFlyVector direction{
            target[0] - state->start[0],
            target[1] - state->start[1],
            target[2] - state->start[2]};
        state->duration = runtime_.DistanceFromVector(direction) /
                          state->speed;
        state->elapsed = 0.0f;
        runtime_.ClampFacingVector(direction);
        direction[1] = 0.0f;
        auto radians = static_cast<double>(
            runtime_.AngleFromZ(direction[2]));
        if (direction[0] < 0.0f)
            radians = 6.283185307179586 - radians;
        runtime_.SetRotation(npc,
            {0.0f, static_cast<float>(radians * 57.29578), 0.0f}, 0u);
        runtime_.FlushWait(*state);
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    return runtime_.CleanupName();
}

void ScriptNpcFlyMove::Update(NpcFlyWaitState& state, double delta) {
    if (state.duration <= 0.0f) {
        runtime_.CloseWait(state);
        return;
    }
    state.elapsed = static_cast<float>(
        static_cast<double>(state.elapsed) + delta);
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        runtime_.CloseWait(state);
    }
    const auto ratio = state.elapsed / state.duration;
    NpcFlyVector position{};
    for (std::size_t i = 0; i < position.size(); ++i)
        position[i] = runtime_.Interpolate(state.start[i],
                                           state.target[i], ratio);
    runtime_.SetPosition(state.npc, position);
}

std::int32_t ScriptNpcFlyMove::Finish(NpcFlyWaitState& state) {
    runtime_.CloseWait(state);
    return runtime_.SetIdleAction(state.npc, "C01");
}

}  // namespace pal4
