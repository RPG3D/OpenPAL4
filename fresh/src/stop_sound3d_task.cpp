#include "pal4/stop_sound3d_task.h"

namespace pal4 {

StopSound3DTaskState& StopSound3DTask::Construct(
    StopSound3DTaskState& state, IStopSound3DTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[18u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

StopSound3DTaskState& StopSound3DTask::Destroy(
    StopSound3DTaskState& state, IStopSound3DTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view StopSound3DTask::ClassName() {
    return "StopSound3DTask";
}

std::uint8_t StopSound3DTask::Start(
    StopSound3DTaskState& state, IStopSound3DTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[18u] = facet;
    runtime.StopSound(facet);
    return 1u;
}

}  // namespace pal4
