#include "pal4/monster_random_point_restore.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>
#include <limits>

namespace pal4 {

std::int32_t MonsterRandomPointRuntimeStub::SoundRuntimeType() { return 3; }
BehaviorVector3 MonsterRandomPointRuntimeStub::CurrentHelperPoint(
    std::uint32_t) { return {}; }
float MonsterRandomPointRuntimeStub::ReadSoundActionTime(
    std::uint32_t, float fallback) { return fallback; }

void MonsterRandomPointRestore::Accept(
    const MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime,
    IMonsterRandomPointRuntime& point_runtime) {
    const auto manager_address = target.words[0x04u / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto config = manager.words[0x6d0u / 4u];
    std::uint32_t path = 0u;
    if (config != 0u) {
        const auto name = runtime.ReadStringAttribute(
            config, "MSTINFO_Behavior_GroupPathWayID", "NOPathWay");
        path = runtime.ResolveWorldPath(name);
    }
    runtime.ConfigurePath(helper, helper_address,
        runtime.GridParameter(grid), path);

    std::uint32_t parameters = 0u;
    for (std::int32_t index = 0; index < runtime.ChildCount(config);
         ++index) {
        const auto child = runtime.Child(config,
            static_cast<std::uint32_t>(index));
        if (runtime.ReadIntAttribute(child,
                "MSTINFO_Behavior_ObjectType", -1) != 3)
            continue;
        for (std::int32_t nested = 0; nested < runtime.ChildCount(child);
             ++nested) {
            const auto candidate = runtime.Child(child,
                static_cast<std::uint32_t>(nested));
            if (runtime.ChildName(candidate) == "AAB_MST_parameters") {
                parameters = candidate;
                break;
            }
        }
        if (parameters != 0u)
            break;
    }
    if (parameters == 0u)
        return;

    const auto max_speed = runtime.ReadFloatAttribute(parameters,
        "AABWander_MST_max_speed", 0.0f);
    const auto max_force = runtime.ReadFloatAttribute(parameters,
        "AABWander_MST_max_force", 0.0f);
    const auto max_range = runtime.ReadFloatAttribute(parameters,
        "AABWander_MST_max_range", 0.0f);
    runtime.SetHelperScalar(helper_address, 0x7cu, max_speed);
    runtime.SetHelperScalar(helper_address, 0x74u, max_force);
    runtime.SetHelperScalar(helper_address, 0x68u, max_speed);
    runtime.SetHelperScalar(helper_address, 0x5cu,
        std::bit_cast<float>(manager.words[0xdcu / 4u]) * 1.5f);

    const auto memory = manager.words[0x668u / 4u];
    const auto last_behavior = runtime.ReadMemoryInt(memory,
        "AAB_MST_memory_last_behavior_id", 0x7fffffff);
    const auto memory_speed = runtime.ReadMemoryFloat(memory,
        "AAB_MST_memory_speed", 0.0f);
    const BehaviorVector3 saved_point{
        runtime.ReadMemoryFloat(memory,
            "AABWander_MST_memory_start_x", std::numeric_limits<float>::max()),
        runtime.ReadMemoryFloat(memory,
            "AABWander_MST_memory_start_y", std::numeric_limits<float>::max()),
        runtime.ReadMemoryFloat(memory,
            "AABWander_MST_memory_start_z", std::numeric_limits<float>::max())};
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
    if (last_behavior == point_runtime.SoundRuntimeType()) {
        runtime.SetAnimationTime(animation,
            point_runtime.ReadSoundActionTime(memory, 0.0f));
    }
}

}  // namespace pal4
