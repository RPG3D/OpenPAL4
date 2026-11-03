#include "pal4/ai_action_task.h"

#include <array>
#include <vector>

namespace pal4 {
namespace {

struct PipelineOption {
    AiPipelineKind kind;
    std::uint32_t capability;
    std::uint32_t lookup_slot;
    std::string_view message;
};

constexpr std::array<PipelineOption, 3> kOptions{{
    {AiPipelineKind::Stunt, 2u, 44u,
     "selectStuntPipeline, stunt:%s"},
    {AiPipelineKind::Magic, 3u, 40u,
     "selectMagicPipeline, magic:%s"},
    {AiPipelineKind::Property, 4u, 48u,
     "selectPropertyPipeline, Property:%s"},
}};

std::uint32_t Select(const std::uint32_t task,
                     const std::uint32_t selected_target,
                     const std::uint32_t mode,
                     const std::size_t option_count,
                     const std::string_view source,
                     IAiActionPipelineRuntime& runtime) {
    std::vector<std::uint32_t> candidates;
    const auto actor = runtime.ReadWord(task + 4u);
    try {
        for (std::size_t index = 0u; index < option_count; ++index) {
            const auto& option = kOptions[index];
            if (!runtime.ActorCapability(actor, option.capability)) continue;
            const auto pipeline = runtime.LookupPipeline(
                task, option.lookup_slot, mode);
            if (pipeline == 0u) continue;
            if (option.kind != AiPipelineKind::Property ||
                runtime.HasFirstProperty(pipeline)) {
                runtime.LogPipeline(task, source, option.message,
                                    runtime.PipelineName(pipeline, option.kind));
            }
            const auto candidate = runtime.SelectCandidate(
                task, option.kind, pipeline, selected_target);
            if (candidate != 0u) candidates.push_back(candidate);
        }

        if (candidates.empty()) {
            runtime.LogPipeline(
                task, source,
                "can't find the appropriate stunt/magic/property, doNormalAttackAction",
                {});
            return runtime.FallbackNormal(task, selected_target);
        }

        const auto selected_index = runtime.RandomNumber() % candidates.size();
        for (std::uint32_t index = 0u; index < candidates.size(); ++index) {
            if (index == selected_index) continue;
            const auto candidate = candidates[index];
            candidates[index] = 0u;
            runtime.ReleaseCandidate(candidate);
        }
        const auto selected = candidates[selected_index];
        runtime.LogPipeline(task, source, "fint pipeline:%s",
                            runtime.CandidateName(selected));
        candidates[selected_index] = 0u;
        return selected;
    } catch (...) {
        for (const auto candidate : candidates) {
            if (candidate != 0u) runtime.ReleaseCandidate(candidate);
        }
        throw;
    }
}

}  // namespace

std::uint32_t AiActionPipeline::SelectCure(
    const std::uint32_t task, const std::uint32_t selected_target,
    IAiActionPipelineRuntime& runtime) {
    return Select(task, selected_target, 2u, 3u,
                  "cbActorAi::doCureAction", runtime);
}

std::uint32_t AiActionPipeline::SelectMagicAttack(
    const std::uint32_t task, const std::uint32_t selected_target,
    IAiActionPipelineRuntime& runtime) {
    return Select(task, selected_target, 0u, 3u,
                  "cbActorAi::doMagicAttackAction", runtime);
}

std::uint32_t AiActionPipeline::SelectOther(
    const std::uint32_t task, const std::uint32_t selected_target,
    IAiActionPipelineRuntime& runtime) {
    return Select(task, selected_target, 1u, 3u,
                  "cbActorAi::doOtherAction", runtime);
}

std::uint32_t AiActionPipeline::SelectSummon(
    const std::uint32_t task, const std::uint32_t selected_target,
    IAiActionPipelineRuntime& runtime) {
    return Select(task, selected_target, 4u, 2u,
                  "cbActorAi::doSummonAction", runtime);
}

std::uint32_t AiActionPipeline::SelectReviveMate(
    const std::uint32_t task, const std::uint32_t selected_target,
    IAiActionPipelineRuntime& runtime) {
    return Select(task, selected_target, 5u, 2u,
                  "cbActorAi::doSummonAction", runtime);
}

}  // namespace pal4
