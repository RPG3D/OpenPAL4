#include "pal4/monster_path_endpoint_restore.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>

namespace pal4 {
namespace {

void WriteByte(BehaviorPathEndpointState& state,
               std::uint32_t offset, std::uint8_t value) {
    auto& word = state.words[offset / 4u];
    const auto shift = (offset % 4u) * 8u;
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

struct RestoreVariant {
    std::uint32_t manager_offset;
    std::int32_t object_type;
    std::string_view speed_key;
    std::string_view force_key;
    std::string_view animation;
};

constexpr RestoreVariant kC02{0x1cu, 0,
    "AABPedestrian_MST_walk_max_speed",
    "AABPedestrian_MST_walk_max_force", "C02"};
constexpr RestoreVariant kC03{0x14u, 1,
    "AABPedestrian_MST_run_max_speed",
    "AABPedestrian_MST_run_max_force", "C03"};

void AcceptVariant(MonsterPathEndpointTargetState& target,
                   const MonsterBehaviorManagerState& manager,
                   BehaviorPathEndpointState& helper,
                   std::uint32_t helper_address,
                   IMonsterPathEndpointRestoreRuntime& runtime,
                   const RestoreVariant& variant);

}  // namespace

std::string MonsterPathEndpointRestoreRuntimeStub::ReadStringAttribute(
    std::uint32_t, std::string_view, std::string_view fallback) {
    return std::string(fallback);
}
std::uint32_t MonsterPathEndpointRestoreRuntimeStub::ResolveWorldPath(
    std::string_view) { return 0u; }
std::uint32_t MonsterPathEndpointRestoreRuntimeStub::GridParameter(
    std::uint32_t) { return 0u; }
void MonsterPathEndpointRestoreRuntimeStub::ConfigurePath(
    BehaviorPathEndpointState& helper, std::uint32_t,
    std::uint32_t grid_parameter, std::uint32_t path_address) {
    (void)BehaviorPathEndpointHelper::SetPathAndCacheEndpoints(
        helper, grid_parameter, path_address, endpoint_);
}
std::int32_t MonsterPathEndpointRestoreRuntimeStub::ChildCount(
    std::uint32_t) { return 0; }
std::uint32_t MonsterPathEndpointRestoreRuntimeStub::Child(
    std::uint32_t, std::uint32_t) { return 0u; }
std::string MonsterPathEndpointRestoreRuntimeStub::ChildName(
    std::uint32_t) { return {}; }
std::int32_t MonsterPathEndpointRestoreRuntimeStub::ReadIntAttribute(
    std::uint32_t, std::string_view, std::int32_t fallback) {
    return fallback;
}
float MonsterPathEndpointRestoreRuntimeStub::ReadFloatAttribute(
    std::uint32_t, std::string_view, float fallback) {
    return fallback;
}
std::int32_t MonsterPathEndpointRestoreRuntimeStub::ReadMemoryInt(
    std::uint32_t, std::string_view, std::int32_t fallback) {
    return fallback;
}
float MonsterPathEndpointRestoreRuntimeStub::ReadMemoryFloat(
    std::uint32_t, std::string_view, float fallback) {
    return fallback;
}
void MonsterPathEndpointRestoreRuntimeStub::SetHelperScalar(
    std::uint32_t, std::uint32_t, float) {}
MonsterTransformRows MonsterPathEndpointRestoreRuntimeStub::ActorTransform(
    std::uint32_t) { return {}; }
void MonsterPathEndpointRestoreRuntimeStub::SetHelperVector(
    std::uint32_t, std::uint32_t, const BehaviorVector3&) {}
void MonsterPathEndpointRestoreRuntimeStub::AddGridElement(
    std::uint32_t, std::uint32_t) {}
bool MonsterPathEndpointRestoreRuntimeStub::SelectAnimation(
    std::uint32_t, std::string_view) { return false; }
void MonsterPathEndpointRestoreRuntimeStub::SetAnimationFlag(
    std::uint32_t, std::uint32_t) {}
std::int32_t MonsterPathEndpointRestoreRuntimeStub::ScriptVersion() {
    return 0;
}
void MonsterPathEndpointRestoreRuntimeStub::SetAnimationTime(
    std::uint32_t, float) {}
std::uint32_t MonsterPathEndpointRestoreRuntimeStub::AnimationFlag(
    std::uint32_t) { return 0u; }

void MonsterPathEndpointRestore::Accept(
    MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper, std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime) {
    AcceptVariant(target, manager, helper, helper_address, runtime, kC02);
}

void MonsterPathEndpointC03Restore::Accept(
    MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper, std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime) {
    AcceptVariant(target, manager, helper, helper_address, runtime, kC03);
}

namespace {

void AcceptVariant(MonsterPathEndpointTargetState& target,
                   const MonsterBehaviorManagerState& manager,
                   BehaviorPathEndpointState& helper,
                   std::uint32_t helper_address,
                   IMonsterPathEndpointRestoreRuntime& runtime,
                   const RestoreVariant& variant) {
    const auto manager_address = target.words[variant.manager_offset / 4u];
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
        const auto child = runtime.Child(
            config, static_cast<std::uint32_t>(index));
        if (runtime.ReadIntAttribute(child,
                "MSTINFO_Behavior_ObjectType", -1) != variant.object_type)
            continue;
        for (std::int32_t nested = 0; nested < runtime.ChildCount(child);
             ++nested) {
            const auto candidate = runtime.Child(
                child, static_cast<std::uint32_t>(nested));
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

    const auto walk_speed = runtime.ReadFloatAttribute(parameters,
        variant.speed_key, 0.0f);
    const auto walk_force = runtime.ReadFloatAttribute(parameters,
        variant.force_key, 0.0f);
    const auto pathfollow = runtime.ReadIntAttribute(parameters,
        "AABPedestrian_MST_walk_pathfollow", 0);
    runtime.SetHelperScalar(helper_address, 0x7cu, walk_speed);
    runtime.SetHelperScalar(helper_address, 0x74u, walk_force);
    runtime.SetHelperScalar(helper_address, 0x68u, walk_speed);
    runtime.SetHelperScalar(helper_address, 0x5cu,
        std::bit_cast<float>(manager.words[0xdcu / 4u]) * 1.5f);

    const auto memory = manager.words[0x668u / 4u];
    const auto memory_speed = runtime.ReadMemoryFloat(memory,
        "AAB_MST_memory_speed", 0.0f);
    const auto path_direction = runtime.ReadMemoryInt(memory,
        "AABPedestrian_MST_memory_pathDirection", 1);
    const auto wander = runtime.ReadMemoryInt(memory,
        "AABPedestrian_MST_memory_wander", 0);
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
        "AAB_MST_memory_last_behavior_id", 0x7fffffff);
    if (last_behavior == runtime.ScriptVersion()) {
        const auto time = runtime.ReadMemoryFloat(memory,
            "AAB_MST_memory_actiontime", 0.0f);
        runtime.SetAnimationTime(animation, time);
    }
}

}  // namespace

}  // namespace pal4
