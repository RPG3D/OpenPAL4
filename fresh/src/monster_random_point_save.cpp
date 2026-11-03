#include "pal4/monster_random_point_save.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>

namespace pal4 {

std::uint32_t MonsterRandomPointSave::Release(
    const MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime,
    IMonsterRandomPointRuntime& point_runtime) {
    const auto manager_address = target.words[0x04u / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto memory = manager.words[0x668u / 4u];
    const auto animation = manager_address + 0x170u;

    runtime.WriteMemoryInt(memory, "AAB_MST_memory_last_behavior_id",
                           point_runtime.SoundRuntimeType());
    runtime.WriteMemoryFloat(memory, "AAB_MST_memory_speed",
                             runtime.HelperSpeed(helper_address));
    runtime.WriteMemoryFloat(memory, "AAB_MST_memory_actiontime",
                             runtime.AnimationTime(animation));
    runtime.WriteMemoryFloat(memory, "AABWander_MST_memory_start_x",
        std::bit_cast<float>(helper.words[0xd4u / 4u]));
    runtime.WriteMemoryFloat(memory, "AABWander_MST_memory_start_y",
        std::bit_cast<float>(helper.words[0xd8u / 4u]));
    runtime.WriteMemoryFloat(memory, "AABWander_MST_memory_start_z",
        std::bit_cast<float>(helper.words[0xdcu / 4u]));

    helper.words[0xb8u / 4u] &= 0xffffff00u;
    runtime.RemoveGridElement(grid, helper_address);
    const auto first = runtime.SelectAction(animation, "C01", 0.5f);
    return first != 0u ? first
        : runtime.SelectAction(animation, "Z01", 0.5f);
}

}  // namespace pal4
