#include "pal4/monster_animation_flag_restore.h"

#include "pal4/monster_behavior_manager.h"

#include <bit>

namespace pal4 {

void MonsterAnimationFlagRestore::Accept(
    MonsterPathEndpointTargetState& target,
    const MonsterBehaviorManagerState& manager,
    BehaviorPathEndpointState& helper,
    std::uint32_t helper_address,
    IMonsterPathEndpointRestoreRuntime& runtime) {
    const auto manager_address = target.words[0x0cu / 4u];
    const auto grid = manager.words[0x204u / 4u];
    const auto animation = manager_address + 0x170u;
    runtime.ConfigurePath(helper, helper_address,
        runtime.GridParameter(grid), 0u);
    runtime.SetHelperScalar(helper_address, 0x68u, 0.0f);
    runtime.SetHelperScalar(helper_address, 0x5cu,
        std::bit_cast<float>(manager.words[0xdcu / 4u]) * 1.5f);

    const auto rows = runtime.ActorTransform(manager.words[0x14u / 4u]);
    runtime.SetHelperVector(helper_address, 0x14u, rows[2]);
    runtime.SetHelperVector(helper_address, 0x04u, rows[0]);
    runtime.SetHelperVector(helper_address, 0x0cu, rows[1]);
    runtime.SetHelperVector(helper_address, 0x1cu, rows[3]);
    runtime.AddGridElement(grid, helper_address);
    helper.words[0xb8u / 4u] =
        (helper.words[0xb8u / 4u] & 0xffffff00u) | 1u;

    if (!runtime.SelectAnimation(animation, "X") &&
        !runtime.SelectAnimation(animation, "C01"))
        (void)runtime.SelectAnimation(animation, "Z01");
    runtime.SetAnimationFlag(animation, 1u);

    const auto memory = manager.words[0x668u / 4u];
    if (runtime.ReadMemoryInt(memory,
            "AAB_MST_memory_last_behavior_id", 0x7fffffff) ==
        runtime.ScriptVersion()) {
        runtime.SetAnimationTime(animation,
            runtime.ReadMemoryFloat(memory,
                "AAB_MST_memory_actiontime", 0.0f));
    }
}

bool MonsterAnimationFlagRestore::IsDone(
    const MonsterPathEndpointTargetState& target,
    IMonsterPathEndpointRestoreRuntime& runtime) {
    return runtime.AnimationFlag(target.words[0x0cu / 4u] + 0x170u) != 0u;
}

}  // namespace pal4
