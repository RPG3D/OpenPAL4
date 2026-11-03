#include "pal4/script_object_timed_move.h"

#include <string>

namespace pal4 {

std::uint32_t ScriptObjectTimedMove::Create(
    std::string_view object_name, float duration,
    const TimedMovePosition& target, std::uint8_t wait_mode) {
    const auto object = runtime_.FindByName(runtime_.ObjectManager(),
                                            object_name);
    if (object == 0u) return 0u;
    if (!runtime_.HasAnimatedObject(object)) return object;

    const auto name = std::string(object_name) + "-OBJ-Movment";
    auto* state = runtime_.AllocateWait(sizeof(TimedObjectMoveWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        if (duration > 0.0f) {
            state->phase = 5u;
            runtime_.AssignName(*state, name);
            state->object = object;
            state->target = target;
            state->start = runtime_.ReadPosition(object);
            state->duration = duration;
            state->elapsed = 0.0f;
            runtime_.FlushWait(*state);
        }
    }
    runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset, state);
    return runtime_.CleanupName();
}

void ScriptObjectTimedMove::Update(TimedObjectMoveWaitState& state,
                                   double delta) {
    if (state.duration <= 0.0f ||
        !runtime_.HasAnimatedObject(state.object)) {
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
    TimedMovePosition current{};
    for (std::size_t i = 0; i < current.size(); ++i)
        current[i] = runtime_.Interpolate(state.start[i],
                                          state.target[i], ratio);
    runtime_.SetPosition(state.object, current, 0u);
}

}  // namespace pal4
