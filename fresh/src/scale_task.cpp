#include "pal4/scale_task.h"

#include "scale_task_state.h"

namespace pal4 {
using namespace scale_task_detail;

ScaleTaskState& ScaleTask::Construct(ScaleTaskState& state,
                                      IScaleTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[0u] = kVtable;
    WriteFlag(state, 72u, false);
    state.words[25u] = 0u;
    return state;
}

ScaleTaskState& ScaleTask::Destroy(ScaleTaskState& state,
                                    IScaleTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view ScaleTask::ClassName() { return "ScaleTask"; }

bool ScaleTask::IsDurationComplete(const ScaleTaskState& state,
                                    IScaleTaskRuntime& runtime) {
    return !ReadFlag(state, 104u) && runtime.BaseDurationComplete(state);
}

std::uint8_t ScaleTask::Start(ScaleTaskState& state,
                               IScaleTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[25u] = facet;
    auto origin = ReadVector(state, 36u);
    runtime.ReadScale(facet, origin);
    WriteVector(state, 36u, origin);

    state.words[28u] = state.words[5u];
    if (ReadFlag(state, 104u))
        WriteFloat(state, 28u, BiasedDuration(state, runtime));
    if (ReadFlag(state, 72u)) {
        const auto reciprocal = 1.0f / ReadFloat(state, 28u);
        for (std::size_t i = 0; i < 3u; ++i)
            WriteFloat(state, 19u + i,
                       (ReadFloat(state, 22u + i) - origin[i]) * reciprocal);
    } else {
        for (std::size_t i = 0; i < 3u; ++i)
            state.words[19u + i] = state.words[22u + i];
    }
    WriteFlag(state, 105u, false);
    state.words[27u] = 0u;
    WriteFlag(state, 120u, false);
    state.words[32u] = 0u;
    WriteFlag(state, 132u, false);
    state.words[35u] = 0u;
    return 1u;
}

std::uint8_t ScaleTask::Stop(ScaleTaskState& state,
                              IScaleTaskRuntime& runtime) {
    if (ReadFlag(state, 72u))
        runtime.SetScale(state.words[25u], ReadVector(state, 22u), 0u);
    state.words[25u] = 0u;
    return 1u;
}

}  // namespace pal4
