#include "pal4/monster_animation_flag_save.h"

#include "pal4/monster_behavior_manager.h"

namespace pal4 {

std::int32_t MonsterAnimationFlagSaveRuntimeStub::ScriptVersion() {
    return 0;
}
void MonsterAnimationFlagSaveRuntimeStub::WriteMemoryInt(
    std::uint32_t, std::string_view, std::int32_t) {}
float MonsterAnimationFlagSaveRuntimeStub::AnimationTime(
    std::uint32_t) { return 0.0f; }
void MonsterAnimationFlagSaveRuntimeStub::WriteMemoryFloat(
    std::uint32_t, std::string_view, float) {}
void MonsterAnimationFlagSaveRuntimeStub::RemoveGridElement(
    std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterAnimationFlagSaveRuntimeStub::SelectAnimation(
    std::uint32_t, std::string_view) { return 0u; }

std::uint32_t MonsterAnimationFlagSave::Release(
    const MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterAnimationFlagSaveRuntime& runtime) {
    const auto manager_address = target.words[0x0cu / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto memory = manager.words[0x668u / 4u];
    const auto animation = manager_address + 0x170u;
    runtime.WriteMemoryInt(memory, "AAB_MST_memory_last_behavior_id",
                           runtime.ScriptVersion());
    runtime.WriteMemoryFloat(memory, "AAB_MST_memory_actiontime",
                             runtime.AnimationTime(animation));
    helper.words[0xb8u / 4u] &= 0xffffff00u;
    runtime.RemoveGridElement(grid, helper_address);
    const auto first = runtime.SelectAnimation(animation, "C01");
    return first != 0u ? first
        : runtime.SelectAnimation(animation, "Z01");
}

}  // namespace pal4
