#include "pal4/ai_action_task.h"

namespace pal4 {

std::uint32_t AiDirectAction::NormalAttack(
    const std::uint32_t task, const std::uint32_t requested_target,
    IAiNormalAttackRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 4u);
    if (!runtime.ActorCapability(actor, 0u)) return 0u;

    const auto actor_targets = runtime.AcquireActorTargets(actor);
    std::uint32_t pool = 0u;
    try {
        pool = runtime.BuildTargetPool(task, actor_targets);
    } catch (...) {
        if (actor_targets != 0u) runtime.ReleaseActorTargets(actor_targets);
        throw;
    }
    if (actor_targets != 0u) runtime.ReleaseActorTargets(actor_targets);

    std::uint32_t target = 0u;
    try {
        target = runtime.SelectCombatTarget(
            task, pool, requested_target);
    } catch (...) {
        runtime.ReleaseTargetPool(pool);
        throw;
    }
    runtime.ReleaseTargetPool(pool);
    if (target == 0u) return 0u;

    runtime.LogAction(task, "cbActorAi::doNormalAttackAction",
                      "selectTarget: %s", runtime.TargetName(target));
    return runtime.CreateCombatAction(actor, target, 0u);
}

std::uint32_t AiDirectAction::Flee(
    const std::uint32_t task, const std::uint32_t requested_target,
    IAiDirectActionRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 4u);
    if (!runtime.ActorCapability(actor, 5u))
        return runtime.FallbackNormal(task, requested_target);
    runtime.LogAction(task, "cbActorAi::doFleeAction",
                      "return fleePipeline", {});
    return runtime.CreateCombatFlee(actor);
}

}  // namespace pal4
