#include "pal4/npc_curve_follow_target.h"

namespace pal4 {

std::uint32_t NpcCurveFollowTarget::Accept(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime) {
    const auto manager_address = target.words[0x04u / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto config = manager.words[0x6d8u / 4u];
    std::uint32_t path = 0u;
    if (config != 0u) {
        const auto name = runtime.ReadStringAttribute(
            config, "NPCINFO_Behavior_GroupPathWayID", "NOPathWay");
        path = runtime.ResolveWorldPath(name);
    }
    runtime.ConfigurePath(helper, helper_address,
        runtime.GridParameter(grid), path);

    const auto count = runtime.ChildCount(config);
    std::uint32_t parameters = 0u;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto child = runtime.Child(config,
            static_cast<std::uint32_t>(index));
        if (runtime.ReadIntAttribute(child,
                "NPCINFO_Behavior_ObjectType", -1) != 4)
            continue;
        const auto nested_count = runtime.ChildCount(child);
        for (std::int32_t nested = 0; nested < nested_count; ++nested) {
            const auto candidate = runtime.Child(child,
                static_cast<std::uint32_t>(nested));
            if (runtime.ChildName(candidate) == "AAB_NPC_parameters") {
                parameters = candidate;
                break;
            }
        }
        if (parameters != 0u) break;
    }
    if (parameters == 0u)
        return static_cast<std::uint32_t>(count);

    const auto max_speed = runtime.ReadFloatAttribute(parameters,
        "AABSeekOnPath_NPC_walk_max_speed", 0.0f);
    runtime.SetHelperScalar(helper_address, 0x7cu, max_speed);
    runtime.SetHelperScalar(helper_address, 0x68u, max_speed);
    const auto memory = manager.words[0x668u / 4u];
    (void)runtime.ReadMemoryFloat(memory, "AAB_NPC_memory_speed", 0.0f);

    const auto rows = runtime.ActorTransform(manager.words[0x14u / 4u]);
    runtime.SetHelperVector(helper_address, 0x14u, rows[2]);
    runtime.SetHelperVector(helper_address, 0x04u, rows[0]);
    runtime.SetHelperVector(helper_address, 0x0cu, rows[1]);
    runtime.SetHelperVector(helper_address, 0x1cu, rows[3]);
    runtime.AddGridElement(grid, helper_address);
    helper.words[0xb8u / 4u] =
        (helper.words[0xb8u / 4u] & 0xffffff00u) | 1u;

    const auto animation = manager_address + 0x170u;
    if (!runtime.SelectAnimation(animation, "C02"))
        (void)runtime.SelectAnimation(animation, "Z02");
    runtime.SetAnimationFlag(animation, 1u);
    const auto last_behavior = runtime.ReadMemoryInt(memory,
        "AAB_NPC_memory_last_behavior_id", 0x7fffffff);
    if (last_behavior == 4) {
        const auto time = runtime.ReadMemoryFloat(memory,
            "AAB_NPC_memory_actiontime", 0.0f);
        return runtime.SetAnimationTimeAndReturn(animation, time);
    }
    return 4u;
}

std::uint32_t NpcCurveFollowTarget::Release(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointSaveRuntime& runtime) {
    const auto manager_address = target.words[0x04u / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto memory = manager.words[0x668u / 4u];
    const auto animation = manager_address + 0x170u;
    runtime.WriteMemoryInt(memory, "AAB_NPC_memory_last_behavior_id", 4);
    runtime.WriteMemoryFloat(memory, "AAB_NPC_memory_speed",
                             runtime.HelperSpeed(helper_address));
    runtime.WriteMemoryFloat(memory, "AAB_NPC_memory_actiontime",
                             runtime.AnimationTime(animation));
    helper.words[0xb8u / 4u] &= 0xffffff00u;
    runtime.RemoveGridElement(grid, helper_address);
    if (runtime.SelectAction(animation, "C01", 0.5f) == 0u)
        (void)runtime.SelectAction(animation, "Z01", 0.5f);
    return runtime.SetAnimationFlag(animation, 1u);
}

}  // namespace pal4
