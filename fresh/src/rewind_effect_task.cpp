#include "pal4/rewind_effect_task.h"

namespace pal4 {

RewindEffectTaskState& RewindEffectTask::Construct(
    RewindEffectTaskState& state, IRewindEffectTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[18u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

RewindEffectTaskState& RewindEffectTask::Destroy(
    RewindEffectTaskState& state, IRewindEffectTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view RewindEffectTask::ClassName() {
    return "RewindEffectTask";
}

std::uint8_t RewindEffectTask::Start(
    RewindEffectTaskState& state, IRewindEffectTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[18u] = facet;
    runtime.Rewind(facet);
    return 1u;
}

}  // namespace pal4
