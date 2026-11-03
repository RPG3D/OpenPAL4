#include "pal4/pause_effect_task.h"

namespace pal4 {

PauseEffectTaskState& PauseEffectTask::Construct(
    PauseEffectTaskState& state, IPauseEffectTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[18u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

PauseEffectTaskState& PauseEffectTask::Destroy(
    PauseEffectTaskState& state, IPauseEffectTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view PauseEffectTask::ClassName() {
    return "PauseEffectTask";
}

std::uint8_t PauseEffectTask::Start(
    PauseEffectTaskState& state, IPauseEffectTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[18u] = facet;
    runtime.Pause(facet);
    return 1u;
}

}  // namespace pal4
