#include "pal4/start_camera_task.h"

namespace pal4 {

StartCameraTaskState& StartCameraTask::Construct(
    StartCameraTaskState& state, IStartCameraTaskRuntime& runtime) {
    runtime.ConstructBase(state);
    state.words[18u] = 0u;
    state.words[0u] = kVtable;
    return state;
}

StartCameraTaskState& StartCameraTask::Destroy(
    StartCameraTaskState& state, IStartCameraTaskRuntime& runtime) {
    state.words[0u] = kVtable;
    runtime.DestroyBase(state);
    return state;
}

std::string_view StartCameraTask::ClassName() {
    return "StartCameraTask";
}

std::uint8_t StartCameraTask::Start(
    StartCameraTaskState& state, IStartCameraTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], ClassName());
    state.words[18u] = facet;
    runtime.StartCamera(facet);
    return 1u;
}

}  // namespace pal4
