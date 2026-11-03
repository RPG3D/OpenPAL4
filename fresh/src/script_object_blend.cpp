#include "pal4/script_object_blend.h"

namespace pal4 {

std::uint32_t ScriptObjectBlend::BlendOut(
    std::string_view object_name, float duration, std::uint8_t wait_mode) {
    return Create(object_name, duration, wait_mode, false);
}

std::uint32_t ScriptObjectBlend::BlendIn(
    std::string_view object_name, float duration, std::uint8_t wait_mode) {
    return Create(object_name, duration, wait_mode, true);
}

std::uint32_t ScriptObjectBlend::Create(
    std::string_view object_name, float duration,
    std::uint8_t wait_mode, bool fade_in) {
    const auto object = runtime_.FindByName(runtime_.ObjectManager(),
                                            object_name);
    if (object == 0u) return 0u;
    auto* state = runtime_.AllocateWait(sizeof(ObjectBlendWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->object = object;
        state->duration = duration;
        state->elapsed = 0.0f;
        state->vtable_address = fade_in ? kInVtable : kOutVtable;
        runtime_.FlushWait(*state);
        if (fade_in) {
            runtime_.RemoveAnimatedObject(object);
            runtime_.SetVisible(object, true);
            runtime_.AddAnimatedObject(object);
            runtime_.SetMaterialFlags(object, 0x00ffffffu);
        } else {
            runtime_.SetMaterialFlags(object, 0xffffffffu);
        }
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptObjectBlend::UpdateOut(ObjectBlendWaitState& state,
                                  float delta) {
    if (state.duration <= 0.0f ||
        !runtime_.HasAnimatedObject(state.object)) {
        runtime_.CloseWait(state);
        return;
    }
    state.elapsed += delta;
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        runtime_.SetMaterialFlags(state.object, 0xffffffffu);
        runtime_.RemoveAnimatedObject(state.object);
        runtime_.CloseWait(state);
        return;
    }
    SetAlpha(state.object, 255.0f, 0.0f,
             state.elapsed / state.duration);
}

void ScriptObjectBlend::FinishOut(ObjectBlendWaitState& state) {
    if (runtime_.HasAnimatedObject(state.object)) {
        runtime_.SetMaterialFlags(state.object, 0xffffffffu);
        runtime_.RemoveAnimatedObject(state.object);
    }
    runtime_.CloseWait(state);
}

void ScriptObjectBlend::UpdateIn(ObjectBlendWaitState& state,
                                 float delta) {
    state.elapsed += delta;
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        runtime_.SetMaterialFlags(state.object, 0xffffffffu);
        runtime_.CloseWait(state);
        return;
    }
    SetAlpha(state.object, 0.0f, 255.0f,
             state.elapsed / state.duration);
}

void ScriptObjectBlend::SetAlpha(std::uint32_t object, float start,
                                 float end, float ratio) {
    const auto alpha = runtime_.InterpolateAlpha(start, end, ratio);
    runtime_.SetMaterialFlags(object, 0x00ffffffu |
        (static_cast<std::uint32_t>(alpha) << 24u));
}

}  // namespace pal4
