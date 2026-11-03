#include "pal4/rump_shahdow_task.h"

#include "rump_shahdow_task_state.h"

namespace pal4 {
using namespace rump_shahdow_detail;

bool RumpShahdowTask::LoadConfig(
    RumpShahdowTaskState& state, const std::uint32_t config,
    IRumpShahdowConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    state.words[23u] = 0u;
    auto lifetime = runtime.ReadFloat(
        config, "Attr_Task_RumpShadow_Life", 0.5f);
    auto fps = runtime.ReadFloat(config, "Attr_Task_RumpShadow_Fps", 24.0f);
    auto maximum = runtime.ReadInt(
        config, "Attr_Task_RumpShadow_MaxCount", 5);
    WriteFlag(state, 112u,
              runtime.ReadInt(config, "Attr_Task_RumpShadow_CheckChange", 0) > 0);
    if (maximum < 0)
        maximum = 0;
    if (lifetime < 0.0f)
        lifetime = 0.5f;
    if (fps <= 0.0f)
        fps = 24.0f;
    WriteFloat(state, 20u, lifetime);
    WriteFloat(state, 21u, 1.0f / fps);
    state.words[23u] = static_cast<std::uint32_t>(maximum);
    return true;
}

}  // namespace pal4
