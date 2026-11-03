#include "pal4/ai_action_task.h"

namespace pal4 {

std::uint32_t AiFixedSkillAction::Select(
    const std::uint32_t task, const std::uint32_t target,
    IAiFixedSkillRuntime& runtime) {
    const auto skill_id = runtime.ConvertSkillId(
        runtime.ReadFloat(target + 16u));
    AiPipelineKind kind = AiPipelineKind::Stunt;
    std::uint32_t capability = 2u;
    std::string_view message = "selectStuntPipeline, stunt:%s";
    if (!runtime.HasSkill(kind, skill_id)) {
        kind = AiPipelineKind::Magic;
        capability = 3u;
        message = "selectMagicPipeline, magic:%s";
        if (!runtime.HasSkill(kind, skill_id)) {
            runtime.LogAction(task, "cbActorAi::doSummonAction",
                              "can't find the appropriate stunt/magic/property, doNormalAttackAction",
                              {});
            return runtime.FallbackNormal(task, target);
        }
    }

    const auto actor = runtime.ReadWord(task + 4u);
    if (runtime.ActorCapability(actor, capability)) {
        const auto skill = runtime.FindSkill(kind, skill_id);
        runtime.LogAction(task, "cbActorAi::doSummonAction", message,
                          runtime.SkillName(kind, skill));
        const auto action = runtime.SelectSkill(kind, task, skill, target);
        if (action != 0u) return action;
    }
    runtime.LogAction(task, "cbActorAi::doSummonAction",
                      "can't find the appropriate stunt/magic/property, doNormalAttackAction",
                      {});
    return runtime.FallbackNormal(task, target);
}

}  // namespace pal4
