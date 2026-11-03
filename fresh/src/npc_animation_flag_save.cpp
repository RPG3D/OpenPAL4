#include "pal4/npc_animation_flag_save.h"

namespace pal4 {

std::uint32_t NpcAnimationFlagSave::Release(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterAnimationFlagSaveRuntime& runtime) {
    const auto manager_address = target.words[0x14u / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto memory = manager.words[0x668u / 4u];
    const auto animation = manager_address + 0x170u;
    runtime.WriteMemoryInt(memory, "AAB_NPC_memory_last_behavior_id",
                           runtime.ScriptVersion());
    runtime.WriteMemoryFloat(memory, "AAB_NPC_memory_actiontime",
                             runtime.AnimationTime(animation));
    helper.words[0xb8u / 4u] &= 0xffffff00u;
    runtime.RemoveGridElement(grid, helper_address);
    const auto first = runtime.SelectAnimation(animation, "C01");
    return first != 0u ? first
        : runtime.SelectAnimation(animation, "Z01");
}

}  // namespace pal4
