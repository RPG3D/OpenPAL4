#include "pal4/ai_action_task.h"

#include <vector>

namespace pal4 {
namespace {

std::uint32_t Select(const std::uint32_t task,
                     const std::uint32_t state,
                     const AiPipelineKind kind,
                     IAiSkillRecordCandidateRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 4u);
    const auto capability = kind == AiPipelineKind::Magic ? 3u : 2u;
    if (!runtime.ActorCapability(actor, capability)) return 0u;

    const auto state_offset = kind == AiPipelineKind::Magic ? 8u : 4u;
    std::vector<std::uint32_t> candidates;
    for (std::uint32_t index = 0u; index < 5u; ++index) {
        const auto key = runtime.ReadWord(actor + 896u + 4u * index);
        if (!runtime.HasSkill(kind, key)) continue;
        const auto skill = runtime.FindSkill(kind, key);
        if (runtime.ReadWord(skill + state_offset) == state &&
            runtime.CanUseSkill(actor, kind, skill)) {
            candidates.push_back(skill);
        }
    }
    if (candidates.empty()) return 0u;

    const auto roll = runtime.RandomNumber() % 1000u;
    // IDA56285B-5628C4 (Magic5626BB-562724): interpolate [0,count),
    // then truncate. The decompiler loses count through reused stack slots.
    const auto count = static_cast<float>(candidates.size());
    const auto index = runtime.ConvertToInt(
        static_cast<double>(count) *
        (static_cast<double>(roll) * static_cast<double>(0.001f)));
    return candidates.at(static_cast<std::size_t>(index));
}

}  // namespace

std::uint32_t AiSkillRecordCandidateSelector::SelectMagic(
    const std::uint32_t task, const std::uint32_t state,
    IAiSkillRecordCandidateRuntime& runtime) {
    return Select(task, state, AiPipelineKind::Magic, runtime);
}

std::uint32_t AiSkillRecordCandidateSelector::SelectStunt(
    const std::uint32_t task, const std::uint32_t state,
    IAiSkillRecordCandidateRuntime& runtime) {
    return Select(task, state, AiPipelineKind::Stunt, runtime);
}

}  // namespace pal4
