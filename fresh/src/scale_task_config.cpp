#include "pal4/scale_task.h"

#include "scale_task_state.h"

#include <array>

namespace pal4 {

bool ScaleTask::LoadConfig(ScaleTaskState& state, std::uint32_t config,
                            IScaleTaskConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    scale_task_detail::WriteFlag(state, 72u,
        runtime.ReadInt(config, "Attr_Task_Scale_Result", 0) > 0);
    constexpr std::array<std::string_view, 3u> keys{
        "Attr_Task_Scale_x", "Attr_Task_Scale_y", "Attr_Task_Scale_z"};
    for (std::size_t i = 0; i < keys.size(); ++i)
        scale_task_detail::WriteFloat(state, 22u + i,
            runtime.ReadFloat(config, keys[i], 0.0f));
    scale_task_detail::WriteFlag(state, 61u,
        runtime.ReadInt(config, "Attr_Inherit_Pre_Task", 0) > 0);
    scale_task_detail::WriteFlag(state, 104u,
        runtime.ReadInt(config, "Attr_Task_Scale_Rollback", 0) > 0);
    scale_task_detail::WriteFloat(state, 29u,
        runtime.ReadFloat(config, "Attr_Task_Scale_Time_Bias", 0.0f));
    scale_task_detail::WriteFloat(state, 31u,
        runtime.ReadFloat(config, "Attr_Time_Interval", 0.0f));
    scale_task_detail::WriteFloat(state, 34u,
        runtime.ReadFloat(config, "Attr_Time_Interval2", 0.0f));
    return true;
}

}  // namespace pal4
