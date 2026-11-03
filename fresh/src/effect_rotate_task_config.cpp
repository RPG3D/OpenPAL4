#include "pal4/effect_rotate_task.h"

#include "effect_rotate_task_state.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::string_view, 3u> kRotationKeys{
    "Attr_Task_Rotate_x", "Attr_Task_Rotate_y", "Attr_Task_Rotate_z"};
constexpr std::array<std::string_view, 3u> kAddedVelocityKeys{
    "Attr_Add_Vel_x", "Attr_Add_Vel_y", "Attr_Add_Vel_z"};

}  // namespace

bool EffectRotateTask::LoadConfig(EffectRotateTaskState& state,
                                  const std::uint32_t config,
                                  IEffectRotateConfigRuntime& runtime) {
    if (config == 0u)
        return false;
    state.words[19u] = 0u;
    effect_rotate_detail::WriteFlag(state, 72u,
        runtime.ReadInt(config, "Attr_Task_Rotate_Result", 0) > 0);
    for (std::size_t i = 0; i < 3u; ++i)
        effect_rotate_detail::WriteFloat(state, 20u + i,
            runtime.ReadFloat(config, kRotationKeys[i], 0.0f));
    effect_rotate_detail::WriteFlag(state, 73u,
        runtime.ReadInt(config, "Attr_Add_Vel", 0) > 0);
    for (std::size_t i = 0; i < 3u; ++i)
        effect_rotate_detail::WriteFloat(state, 23u + i,
            runtime.ReadFloat(config, kAddedVelocityKeys[i], 0.0f));
    effect_rotate_detail::WriteFlag(state, 61u,
        runtime.ReadInt(config, "Attr_Inherit_Pre_Task", 0) > 0);
    effect_rotate_detail::WriteFlag(state, 132u,
        runtime.ReadInt(config, "Attr_Task_Rotate_Rollback", 0) > 0);
    effect_rotate_detail::WriteFloat(state, 36u,
        runtime.ReadFloat(config, "Attr_Task_Rotate_Time_Bias", 0.0f));
    effect_rotate_detail::WriteFloat(state, 38u,
        runtime.ReadFloat(config, "Attr_Time_Interval", 0.0f));
    effect_rotate_detail::WriteFloat(state, 41u,
        runtime.ReadFloat(config, "Attr_Time_Interval2", 0.0f));
    effect_rotate_detail::WriteFlag(state, 74u,
        runtime.ReadInt(config, "Attr_Task_Rotate_LockedLookAt", 0) > 0);
    return true;
}

}  // namespace pal4
