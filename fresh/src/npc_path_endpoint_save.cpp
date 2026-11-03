#include "pal4/npc_path_endpoint_save.h"

namespace pal4 {
namespace {

std::uint32_t ReleaseVariant(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime,
    std::uint32_t manager_offset,
    std::int32_t class_id);

}  // namespace

std::uint32_t NpcPathEndpointSave::Release(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime) {
    return ReleaseVariant(target, manager, helper,
                          helper_address, runtime, 0x24u, 0);
}

std::uint32_t NpcPathEndpointC03Save::Release(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime) {
    return ReleaseVariant(target, manager, helper,
                          helper_address, runtime, 0x1cu, 1);
}

namespace {

std::uint32_t ReleaseVariant(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime,
    std::uint32_t manager_offset,
    std::int32_t class_id) {
    const auto manager_address = target.words[manager_offset / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto memory = manager.words[0x668u / 4u];
    const auto animation = manager_address + 0x170u;

    runtime.WriteMemoryInt(memory, "AAB_NPC_memory_last_behavior_id",
                           class_id);
    runtime.WriteMemoryFloat(memory, "AAB_NPC_memory_speed",
                             runtime.HelperSpeed(helper_address));
    runtime.WriteMemoryFloat(memory, "AAB_NPC_memory_actiontime",
                             runtime.AnimationTime(animation));
    runtime.WriteMemoryInt(memory,
        "AABPedestrian_NPC_memory_pathDirection",
        static_cast<std::int32_t>(helper.words[0xc0u / 4u]));
    runtime.WriteMemoryInt(memory,
        "AABPedestrian_NPC_memory_wander",
        static_cast<std::int32_t>(helper.words[0xc4u / 4u] & 0xffu));

    helper.words[0xb8u / 4u] &= 0xffffff00u;
    runtime.RemoveGridElement(grid, helper_address);
    if (runtime.SelectAction(animation, "C01", 0.5f) == 0u)
        (void)runtime.SelectAction(animation, "Z01", 0.5f);
    return runtime.SetAnimationFlag(animation, 1u);
}

}  // namespace

}  // namespace pal4
