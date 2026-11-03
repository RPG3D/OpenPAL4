#include "pal4/ai_action_task.h"

#include <vector>

namespace pal4 {
namespace {

std::uint32_t Select(const std::uint32_t task,
                     const std::uint32_t state,
                     const AiPipelineKind kind,
                     IAiCollectedSkillCandidateRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 4u);
    const auto capability = kind == AiPipelineKind::Magic ? 3u : 2u;
    if (!runtime.ActorCapability(actor, capability)) return 0u;

    const auto collected = runtime.CollectRecords(actor + 920u, state, kind);
    std::vector<std::uint32_t> candidates;
    for (const auto skill : collected) {
        if (!runtime.CanUseSkill(actor, kind, skill)) continue;
        if (kind == AiPipelineKind::Stunt &&
            runtime.ReadWord(skill) == 5509u) continue;
        candidates.push_back(skill);
    }
    if (candidates.empty()) return 0u;

    const auto count = static_cast<float>(candidates.size());
    const auto roll = runtime.RandomNumber() % 1000u;
    const auto index = runtime.ConvertToInt(
        static_cast<double>(roll) * static_cast<double>(0.001f) * count);
    return candidates.at(static_cast<std::size_t>(index));
}

}  // namespace

std::uint32_t AiCollectedSkillCandidateSelector::SelectIndexedMagic(
    const std::uint32_t task, const std::uint32_t state,
    IAiCollectedSkillCandidateRuntime& runtime) {
    return Select(task, state, AiPipelineKind::Magic, runtime);
}

std::uint32_t AiCollectedSkillCandidateSelector::SelectFixedStunt(
    const std::uint32_t task, const std::uint32_t state,
    IAiCollectedSkillCandidateRuntime& runtime) {
    return Select(task, state, AiPipelineKind::Stunt, runtime);
}

}  // namespace pal4
