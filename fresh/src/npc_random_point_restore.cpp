#include "pal4/npc_random_point_restore.h"

#include <bit>
#include <limits>

namespace pal4 {

std::uint32_t NpcRandomPointRestore::Accept(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime,
    IMonsterRandomPointRuntime& point_runtime) {
    const auto manager_address = target.words[0x0cu / 4u];
    const auto grid = manager.words[0x204u / 4u];
    runtime.ConfigurePath(helper, helper_address,
        runtime.GridParameter(grid), 0u);

    const auto config = manager.words[0x6d8u / 4u];
    const auto count = runtime.ChildCount(config);
    std::uint32_t parameters = 0u;
    for (std::int32_t index = 0; index < count; ++index) {
        const auto child = runtime.Child(config,
            static_cast<std::uint32_t>(index));
        if (runtime.ReadIntAttribute(child,
                "NPCINFO_Behavior_ObjectType", -1) != 3)
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
        "AABWander_NPC_max_speed", 0.0f);
    const auto max_force = runtime.ReadFloatAttribute(parameters,
        "AABWander_NPC_max_force", 0.0f);
    const auto max_range = runtime.ReadFloatAttribute(parameters,
        "AABWander_NPC_max_range", 0.0f);
    runtime.SetHelperScalar(helper_address, 0x7cu, max_speed);
    runtime.SetHelperScalar(helper_address, 0x74u, max_force);
    runtime.SetHelperScalar(helper_address, 0x68u, max_speed);
    runtime.SetHelperScalar(helper_address, 0x5cu,
        std::bit_cast<float>(manager.words[0xdcu / 4u]) * 1.5f);

    const auto memory = manager.words[0x668u / 4u];
    const auto last_behavior = runtime.ReadMemoryInt(memory,
        "AAB_NPC_memory_last_behavior_id", 0x7fffffff);
    const auto memory_speed = runtime.ReadMemoryFloat(memory,
        "AAB_NPC_memory_speed", 0.0f);
    const BehaviorVector3 saved_point{
        runtime.ReadMemoryFloat(memory, "AABWander_NPC_memory_start_x",
            std::numeric_limits<float>::max()),
        runtime.ReadMemoryFloat(memory, "AABWander_NPC_memory_start_y",
            std::numeric_limits<float>::max()),
        runtime.ReadMemoryFloat(memory, "AABWander_NPC_memory_start_z",
            std::numeric_limits<float>::max())};
    runtime.SetHelperScalar(helper_address, 0x68u, memory_speed);

    const auto rows = runtime.ActorTransform(manager.words[0x14u / 4u]);
    runtime.SetHelperVector(helper_address, 0x14u, rows[2]);
    runtime.SetHelperVector(helper_address, 0x04u, rows[0]);
    runtime.SetHelperVector(helper_address, 0x0cu, rows[1]);
    runtime.SetHelperVector(helper_address, 0x1cu, rows[3]);

    if (last_behavior == point_runtime.SoundRuntimeType()) {
        if (saved_point[0] < std::numeric_limits<float>::max() &&
            saved_point[1] < std::numeric_limits<float>::max() &&
            saved_point[2] < std::numeric_limits<float>::max()) {
            for (std::uint32_t i = 0; i < 3u; ++i)
                helper.words[0xd4u / 4u + i] =
                    std::bit_cast<std::uint32_t>(saved_point[i]);
        }
    } else {
        const auto point = point_runtime.CurrentHelperPoint(helper_address);
        for (std::uint32_t i = 0; i < 3u; ++i)
            helper.words[0xd4u / 4u + i] =
                std::bit_cast<std::uint32_t>(point[i]);
    }
    helper.words[0xbcu / 4u] = std::bit_cast<std::uint32_t>(max_range);
    runtime.AddGridElement(grid, helper_address);
    helper.words[0xb8u / 4u] =
        (helper.words[0xb8u / 4u] & 0xffffff00u) | 1u;

    const auto animation = manager_address + 0x170u;
    if (!runtime.SelectAnimation(animation, "C02"))
        (void)runtime.SelectAnimation(animation, "Z02");
    runtime.SetAnimationFlag(animation, 1u);
    const auto type = point_runtime.SoundRuntimeType();
    if (last_behavior == type) {
        const auto time = runtime.ReadMemoryFloat(memory,
            "AAB_NPC_memory_actiontime", 0.0f);
        return runtime.SetAnimationTimeAndReturn(animation, time);
    }
    return static_cast<std::uint32_t>(type);
}

}  // namespace pal4
