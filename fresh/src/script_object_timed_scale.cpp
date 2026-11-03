#include "pal4/script_object_timed_scale.h"

namespace pal4 {

std::uint32_t ScriptObjectTimedScale::Create(
    std::string_view object_name, float target_scale,
    float duration, std::uint8_t wait_mode) {
    const auto object = runtime_.FindByName(runtime_.ObjectManager(),
                                            object_name);
    if (object == 0u || !runtime_.HasAnimatedObject(object)) return 0u;

    auto* state = runtime_.AllocateWait(sizeof(TimedObjectScaleWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->vtable_address = kWaitVtable;
        if (duration > 0.0f) {
            state->object = object;
            state->target = {target_scale, target_scale, target_scale};
            state->start = runtime_.ReadScale(object);
            state->duration = duration;
            state->elapsed = 0.0f;
            runtime_.FlushWait(*state);
        }
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptObjectTimedScale::Update(TimedObjectScaleWaitState& state,
                                    double delta) {
    if (state.duration <= 0.0f) return;
    state.elapsed = static_cast<float>(
        static_cast<double>(state.elapsed) + delta);
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        runtime_.CloseWait(state);
    }
    const auto ratio = state.elapsed / state.duration;
    TimedMovePosition scale{};
    for (std::size_t i = 0; i < scale.size(); ++i)
        scale[i] = runtime_.Interpolate(state.start[i],
                                        state.target[i], ratio);
    runtime_.SetScale(state.object, scale, 0u);
}

}  // namespace pal4
