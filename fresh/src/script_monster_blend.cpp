#include "pal4/script_monster_blend.h"

namespace pal4 {

std::uint32_t ScriptMonsterBlend::BlendOut(
    std::string_view monster_name, float duration,
    std::uint8_t wait_mode) {
    return Create(monster_name, duration, wait_mode, false);
}

std::uint32_t ScriptMonsterBlend::BlendIn(
    std::string_view monster_name, float duration,
    std::uint8_t wait_mode) {
    return Create(monster_name, duration, wait_mode, true);
}

std::uint32_t ScriptMonsterBlend::Create(
    std::string_view monster_name, float duration,
    std::uint8_t wait_mode, bool fade_in) {
    const auto monster = runtime_.FindMonster(runtime_.MonsterManager(),
                                              monster_name);
    if (monster == 0u) return 0u;
    auto* state = runtime_.AllocateWait(sizeof(ObjectBlendWaitState));
    if (state != nullptr) {
        runtime_.ConstructWait(*state, wait_mode,
                               runtime_.ScriptExecutor() + kQueueOffset);
        state->object = monster;
        state->duration = duration;
        state->elapsed = 0.0f;
        state->vtable_address = fade_in ? kInVtable : kOutVtable;
        runtime_.FlushWait(*state);
        if (fade_in) {
            const auto info = runtime_.FindMonsterInfo(
                runtime_.MonsterManager(), runtime_.MonsterName(monster));
            runtime_.LeaveCharacter(monster);
            runtime_.SetStateFlag(monster, 1u);
            runtime_.EnterCharacter(monster);
            if (!runtime_.SetAction(monster, "C01", 0.5f))
                static_cast<void>(runtime_.SetAction(monster, "Z01", 0.5f));
            if (info != 0u) runtime_.ReplaceComponent(monster, info, 1u);
            runtime_.SetMaterialFlags(monster, 0x00ffffffu);
        } else {
            runtime_.SetMaterialFlags(monster, 0xffffffffu);
        }
    }
    return runtime_.AddHandler(runtime_.ScriptExecutor() + kQueueOffset,
                               state);
}

void ScriptMonsterBlend::UpdateOut(ObjectBlendWaitState& state,
                                   float delta) {
    state.elapsed += delta;
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        runtime_.SetMaterialFlags(state.object, 0xffffffffu);
        runtime_.LeaveCharacter(state.object);
        runtime_.CloseWait(state);
        return;
    }
    const auto alpha = runtime_.InterpolateAlpha(
        255.0f, 0.0f, state.elapsed / state.duration);
    runtime_.SetMaterialFlags(state.object, 0x00ffffffu |
        (static_cast<std::uint32_t>(alpha) << 24u));
}

void ScriptMonsterBlend::FinishOut(ObjectBlendWaitState& state) {
    runtime_.SetMaterialFlags(state.object, 0xffffffffu);
    runtime_.LeaveCharacter(state.object);
    runtime_.CloseWait(state);
}

void ScriptMonsterBlend::UpdateIn(ObjectBlendWaitState& state,
                                  float delta) {
    state.elapsed += delta;
    if (state.elapsed >= state.duration) {
        state.elapsed = state.duration;
        runtime_.SetMaterialFlags(state.object, 0xffffffffu);
        runtime_.CloseWait(state);
        return;
    }
    const auto alpha = runtime_.InterpolateAlpha(
        0.0f, 255.0f, state.elapsed / state.duration);
    runtime_.SetMaterialFlags(state.object, 0x00ffffffu |
        (static_cast<std::uint32_t>(alpha) << 24u));
}

void ScriptMonsterBlend::FinishIn(ObjectBlendWaitState& state) {
    runtime_.SetMaterialFlags(state.object, 0xffffffffu);
    runtime_.CloseWait(state);
}

}  // namespace pal4
