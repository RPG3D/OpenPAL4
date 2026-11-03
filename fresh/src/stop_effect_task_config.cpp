#include "pal4/stop_effect_task.h"

#include "stop_effect_task_state.h"

namespace pal4 {

bool StopEffectTask::LoadConfig(
    StopEffectTaskState& state, std::uint32_t config,
    IStopEffectConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    stop_effect_detail::WriteFlag(state, 72u,
        runtime.ReadInt(config, "Attr_Task_StopEffect_Smooth", 0) > 0);
    state.words[19u] = 0u;
    state.words[20u] = 0u;
    return true;
}

}  // namespace pal4
