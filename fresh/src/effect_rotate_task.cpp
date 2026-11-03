#include "pal4/effect_rotate_task.h"

#include "effect_rotate_task_state.h"

namespace pal4 {
using namespace effect_rotate_detail;

EffectRotateTaskState& EffectRotateTask::Construct(
    EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    WriteFlag(state, 72u, false);
    state.words[19u] = 0u;
    state.words[32u] = 0u;
    return state;
}

EffectRotateTaskState& EffectRotateTask::Destroy(
    EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

EffectRotateTaskState& EffectRotateTask::Delete(
    EffectRotateTaskState& state, const std::uint8_t flags,
    IEffectRotateTaskRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.Free(state);
    return state;
}

std::string_view EffectRotateTask::ClassName() { return "RotateTask"; }

bool EffectRotateTask::IsDurationComplete(
    const EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime) {
    return !ReadFlag(state, 132u) && runtime.BaseDurationComplete(state);
}

std::uint8_t EffectRotateTask::Start(
    EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[32u] = facet;
    auto origin = ReadVector(state, 43u);
    runtime.ReadRotation(facet, origin);
    WriteVector(state, 43u, origin);

    state.words[35u] = state.words[5u];
    if (ReadFlag(state, 132u))
        WriteFloat(state, 35u, BiasedDuration(state, runtime));
    if (ReadFlag(state, 72u)) {
        const auto reciprocal = 1.0f / ReadFloat(state, 35u);
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 26u + i,
                       (ReadFloat(state, 20u + i) - origin[i]) * reciprocal);
    } else {
        for (std::size_t i = 0; i < 3u; ++i)
            state.words[26u + i] = state.words[20u + i];
        if (ReadFlag(state, 73u)) {
            const auto reciprocal = 1.0f / ReadFloat(state, 35u);
            for (std::size_t i = 0; i < 3u; ++i)
                WriteFloat(state, 29u + i,
                           (ReadFloat(state, 23u + i) -
                            ReadFloat(state, 20u + i)) * reciprocal);
        }
    }
    state.words[34u] = 0u;
    WriteFlag(state, 133u, false);
    WriteFlag(state, 148u, false);
    state.words[39u] = 0u;
    WriteFlag(state, 160u, false);
    state.words[42u] = 0u;
    return 1u;
}

std::uint8_t EffectRotateTask::Stop(
    EffectRotateTaskState& state, IEffectRotateTaskRuntime& runtime) {
    if (ReadFlag(state, 72u))
        runtime.ApplyRotation(state.words[32u], ReadVector(state, 23u), false);
    const auto camera = runtime.ResolveFacet(state.words[16u],
                                              "StartCameraTask");
    if (camera != 0u)
        runtime.SetCameraLockedLookAt(camera, ReadFlag(state, 74u));
    state.words[32u] = 0u;
    return 1u;
}

}  // namespace pal4
