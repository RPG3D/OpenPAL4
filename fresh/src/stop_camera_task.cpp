#include "pal4/stop_camera_task.h"

namespace pal4 {

StopCameraTaskState& StopCameraTask::Construct(
    StopCameraTaskState& state, IStopCameraTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[18u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

StopCameraTaskState& StopCameraTask::Destroy(
    StopCameraTaskState& state, IStopCameraTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view StopCameraTask::ClassName() {
    return "StopCameraTask";
}

std::uint8_t StopCameraTask::Start(
    StopCameraTaskState& state, IStopCameraTaskRuntime& runtime) {
    const auto owner = state.words[16u];
    const auto facet = runtime.ResolveFacet(owner, ClassName());
    state.words[18u] = facet;
    runtime.StopCamera(facet);
    if (runtime.GetFrameParent(runtime.GetOwnerFrame(owner)) != 0u)
        runtime.RemoveFrameChild(runtime.GetOwnerFrame(owner));
    return 1u;
}

}  // namespace pal4
