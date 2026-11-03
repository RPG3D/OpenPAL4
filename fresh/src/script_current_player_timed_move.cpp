#include "pal4/script_current_player_timed_move.h"

namespace pal4 {
namespace {

char FoldAscii(char value) {
    return value >= 'A' && value <= 'Z' ?
        static_cast<char>(value + ('a' - 'A')) : value;
}

bool IsC01(std::string_view name) {
    constexpr std::string_view expected = "C01";
    if (name.size() != expected.size()) return false;
    for (std::size_t i = 0; i < name.size(); ++i)
        if (FoldAscii(name[i]) != FoldAscii(expected[i])) return false;
    return true;
}

}  // namespace

std::uint32_t ScriptCurrentPlayerTimedMove::Create(
    float duration, const TimedMovePosition& target,
    std::uint8_t wait_mode) {
    const auto player = runtime_.CurrentPlayer(runtime_.PlayerManager());
    if (player == 0u || !runtime_.HasMovementComponent(player)) return 0u;

    if (!IsC01(runtime_.AnimationName(player))) {
        runtime_.SelectAnimation(player, "C01");
        runtime_.SetAnimationFlag(player, 1u);
    }
    auto* state = runtime_.AllocateWait(sizeof(CurrentPlayerTimedMoveWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        if (duration > 0.0f) {
            state->object = player;
            state->target = target;
            state->start = runtime_.ReadPosition(player);
            state->duration = duration;
            state->elapsed = 0.0f;
            runtime_.FlushWait(*state);
        }
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptCurrentPlayerTimedMove::Update(
    CurrentPlayerTimedMoveWaitState& state, double delta) {
    if (state.duration <= 0.0f) return;
    state.elapsed = static_cast<float>(
        static_cast<double>(state.elapsed) + delta);
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        runtime_.CloseWait(state);
    }
    const auto ratio = state.elapsed / state.duration;
    TimedMovePosition position{};
    for (std::size_t i = 0; i < position.size(); ++i)
        position[i] = runtime_.Interpolate(state.start[i],
                                           state.target[i], ratio);
    runtime_.SetPosition(state.object, position);
}

}  // namespace pal4
