#include "pal4/stop_effect_task.h"

#include "stop_effect_task_state.h"

namespace pal4 {
using namespace stop_effect_detail;

StopEffectTaskState& StopEffectTask::Construct(
    StopEffectTaskState& state, IStopEffectTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[22u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

StopEffectTaskState& StopEffectTask::Destroy(
    StopEffectTaskState& state, IStopEffectTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view StopEffectTask::ClassName() { return "StopEffectTask"; }

bool StopEffectTask::IsDurationComplete(
    const StopEffectTaskState& state, IStopEffectTaskRuntime& runtime) {
    return ReadFlag(state, 72u) ? ReadFlag(state, 84u)
                                : runtime.BaseDurationComplete(state);
}

std::uint8_t StopEffectTask::Start(
    StopEffectTaskState& state, IStopEffectTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[22u] = facet;
    if (ReadFlag(state, 72u)) {
        WriteFloat(state, 19u, runtime.RemainingEffectTime(facet));
        state.words[20u] = 0u;
        WriteFlag(state, 84u, false);
    } else {
        runtime.StopEffect(facet);
    }
    return 1u;
}

std::uint8_t StopEffectTask::Update(
    StopEffectTaskState& state, float elapsed,
    IStopEffectTaskRuntime& runtime) {
    if (!ReadFlag(state, 72u) || ReadFlag(state, 84u))
        return 1u;
    const auto total = elapsed + ReadFloat(state, 20u);
    WriteFloat(state, 20u, total);
    if (total < ReadFloat(state, 19u))
        return 1u;
    runtime.StopEffect(state.words[22u]);
    WriteFlag(state, 84u, true);
    return 1u;
}

std::uint8_t StopEffectTask::Stop(
    StopEffectTaskState& state, IStopEffectTaskRuntime& runtime) {
    runtime.StopEffect(state.words[22u]);
    state.words[22u] = 0u;
    WriteFlag(state, 84u, true);
    return 1u;
}

}  // namespace pal4
