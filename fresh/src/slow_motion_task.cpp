#include "pal4/slow_motion_task.h"

#include <bit>

namespace pal4 {

std::uint8_t SlowMotionTask::Start(
    SlowMotionTaskState& state, ISlowMotionTaskRuntime& runtime) {
    const auto facet = runtime.ResolveFacet(state.words[16u], "MessageTask");
    state.words[19u] = facet;
    const auto id = runtime.ReadOwnerId(state.words[16u], "ID", -1);
    runtime.ApplyScaleTime(facet, id, std::bit_cast<float>(state.words[18u]));
    return 1u;
}

bool SlowMotionTask::LoadConfig(
    SlowMotionTaskState& state, std::uint32_t config,
    ISlowMotionConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    state.words[18u] = std::bit_cast<std::uint32_t>(runtime.ReadFloat(
        config, "Attr_Task_SlowMotion_ScaleTime", 0.0f));
    return true;
}

std::uint32_t SlowMotionTask::Reset(SlowMotionTaskState& state) {
    state.words[18u] = 0u;
    state.words[19u] = 0u;
    return 0u;
}

}  // namespace pal4
