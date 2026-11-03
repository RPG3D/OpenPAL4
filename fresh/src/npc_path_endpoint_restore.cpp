#include "pal4/npc_path_endpoint_restore.h"

#include <bit>
#include <string_view>

namespace pal4 {
namespace {

struct RestoreVariant {
    std::uint32_t manager_offset;
    std::int32_t object_type;
    std::string_view speed_key;
    std::string_view force_key;
    float property_multiplier;
    std::string_view animation;
};

constexpr RestoreVariant kC02{
    0x24u, 0,
    "AABPedestrian_NPC_walk_max_speed",
    "AABPedestrian_NPC_walk_max_force", 1.0f, "C02"};
constexpr RestoreVariant kC03{
    0x1cu, 1,
    "AABPedestrian_NPC_run_max_speed",
    "AABPedestrian_NPC_run_max_force", 1.5f, "C03"};

void WriteByte(BehaviorPathEndpointState& helper,
               std::uint32_t offset, std::uint8_t value) {
    auto& word = helper.words[offset / 4u];
    const auto shift = (offset & 3u) * 8u;
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

std::uint32_t AcceptVariant(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime,
    const RestoreVariant& variant);

}  // namespace

std::uint32_t NpcPathEndpointRestore::Accept(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime) {
    return AcceptVariant(target, manager, helper,
                         helper_address, runtime, kC02);
}

std::uint32_t NpcPathEndpointC03Restore::Accept(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime) {
    return AcceptVariant(target, manager, helper,
                         helper_address, runtime, kC03);
}

namespace {

std::uint32_t AcceptVariant(
    const NpcPathEndpointTargetState& target,
    const GameObjectState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime,
    const RestoreVariant& variant) {
    const auto manager_address = target.words[variant.manager_offset / 4u];
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
                "NPCINFO_Behavior_ObjectType", -1) != variant.object_type)
            continue;
        const auto nested_count = runtime.ChildCount(child);
        for (std::int32_t nested = 0; nested < nested_count; ++nested) {
            const auto candidate = runtime.Child(
                child, static_cast<std::uint32_t>(nested));
            if (runtime.ChildName(candidate) == "AAB_NPC_parameters") {
                parameters = candidate;
                break;
            }
        }
        if (parameters != 0u) break;
    }
    if (parameters == 0u)
        return static_cast<std::uint32_t>(count);

    const auto walk_speed = runtime.ReadFloatAttribute(parameters,
        variant.speed_key, 0.0f);
    const auto walk_force = runtime.ReadFloatAttribute(parameters,
        variant.force_key, 0.0f);
    const auto pathfollow = runtime.ReadIntAttribute(parameters,
        "AABPedestrian_NPC_walk_pathfollow", 0);
    runtime.SetHelperScalar(helper_address, 0x7cu, walk_speed);
    runtime.SetHelperScalar(helper_address, 0x74u, walk_force);
    runtime.SetHelperScalar(helper_address, 0x68u, walk_speed);
    runtime.SetHelperScalar(helper_address, 0x5cu,
        std::bit_cast<float>(manager.words[0xdcu / 4u]) *
            variant.property_multiplier);

    const auto memory = manager.words[0x668u / 4u];
    const auto memory_speed = runtime.ReadMemoryFloat(memory,
        "AAB_NPC_memory_speed", 0.0f);
    const auto path_direction = runtime.ReadMemoryInt(memory,
        "AABPedestrian_NPC_memory_pathDirection", 1);
    const auto wander = runtime.ReadMemoryInt(memory,
        "AABPedestrian_NPC_memory_wander", 0);
    runtime.SetHelperScalar(helper_address, 0x68u, memory_speed);
    helper.words[0xc0u / 4u] =
        static_cast<std::uint32_t>(path_direction);
    WriteByte(helper, 0xc4u, static_cast<std::uint8_t>(wander > 0));
    WriteByte(helper, 0xc5u, static_cast<std::uint8_t>(pathfollow > 0));

    const auto rows = runtime.ActorTransform(manager.words[0x14u / 4u]);
    runtime.SetHelperVector(helper_address, 0x14u, rows[2]);
    runtime.SetHelperVector(helper_address, 0x04u, rows[0]);
    runtime.SetHelperVector(helper_address, 0x0cu, rows[1]);
    runtime.SetHelperVector(helper_address, 0x1cu, rows[3]);
    runtime.AddGridElement(grid, helper_address);
    WriteByte(helper, 0xb8u, 1u);

    const auto animation = manager_address + 0x170u;
    if (!runtime.SelectAnimation(animation, variant.animation))
        (void)runtime.SelectAnimation(animation, "Z02");
    runtime.SetAnimationFlag(animation, 1u);
    const auto last_behavior = runtime.ReadMemoryInt(memory,
        "AAB_NPC_memory_last_behavior_id", 0x7fffffff);
    if (last_behavior == variant.object_type) {
        const auto time = runtime.ReadMemoryFloat(memory,
            "AAB_NPC_memory_actiontime", 0.0f);
        return runtime.SetAnimationTimeAndReturn(animation, time);
    }
    return 0u;
}

}  // namespace

}  // namespace pal4
