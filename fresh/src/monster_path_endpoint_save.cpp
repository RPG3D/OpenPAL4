#include "pal4/monster_path_endpoint_save.h"

#include "pal4/monster_behavior_manager.h"

namespace pal4 {

std::int32_t MonsterPathEndpointSaveRuntimeStub::ScriptVersion() {
    return 0;
}
void MonsterPathEndpointSaveRuntimeStub::WriteMemoryInt(
    std::uint32_t, std::string_view, std::int32_t) {}
void MonsterPathEndpointSaveRuntimeStub::WriteMemoryFloat(
    std::uint32_t, std::string_view, float) {}
float MonsterPathEndpointSaveRuntimeStub::HelperSpeed(
    std::uint32_t) { return 0.0f; }
float MonsterPathEndpointSaveRuntimeStub::AnimationTime(
    std::uint32_t) { return 0.0f; }
void MonsterPathEndpointSaveRuntimeStub::RemoveGridElement(
    std::uint32_t, std::uint32_t) {}
std::uint32_t MonsterPathEndpointSaveRuntimeStub::SelectAction(
    std::uint32_t, std::string_view, float) { return 0u; }
std::uint32_t MonsterPathEndpointSaveRuntimeStub::SetAnimationFlag(
    std::uint32_t, std::uint32_t) { return 0u; }

namespace {

std::uint32_t ReleaseVariant(
    const MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime,
    std::uint32_t manager_offset, bool restore_animation_flag);

}  // namespace

std::uint32_t MonsterPathEndpointSave::Release(
    const MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime) {
    return ReleaseVariant(target, manager, helper, helper_address,
                          runtime, 0x1cu, false);
}

std::uint32_t MonsterPathEndpointC03Save::Release(
    const MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime) {
    return ReleaseVariant(target, manager, helper, helper_address,
                          runtime, 0x14u, true);
}

namespace {

std::uint32_t ReleaseVariant(
    const MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime,
    std::uint32_t manager_offset, bool restore_animation_flag) {
    const auto manager_address = target.words[manager_offset / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto memory = manager.words[0x668u / 4u];
    const auto animation = manager_address + 0x170u;

    runtime.WriteMemoryInt(memory, "AAB_MST_memory_last_behavior_id",
                           runtime.ScriptVersion());
    runtime.WriteMemoryFloat(memory, "AAB_MST_memory_speed",
                             runtime.HelperSpeed(helper_address));
    runtime.WriteMemoryFloat(memory, "AAB_MST_memory_actiontime",
                             runtime.AnimationTime(animation));
    runtime.WriteMemoryInt(memory,
        "AABPedestrian_MST_memory_pathDirection",
        static_cast<std::int32_t>(helper.words[0xc0u / 4u]));
    runtime.WriteMemoryInt(memory,
        "AABPedestrian_MST_memory_wander",
        static_cast<std::int32_t>(
            helper.words[0xc4u / 4u] & 0xffu));

    helper.words[0xb8u / 4u] &= 0xffffff00u;
    runtime.RemoveGridElement(grid, helper_address);
    const auto first = runtime.SelectAction(animation, "C01", 0.5f);
    const auto selected = first != 0u ? first
        : runtime.SelectAction(animation, "Z01", 0.5f);
    if (restore_animation_flag)
        return runtime.SetAnimationFlag(animation, 1u);
    return selected;
}

}  // namespace

}  // namespace pal4
