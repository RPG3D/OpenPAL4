#include "pal4/ai_action_task.h"

namespace pal4 {
namespace {

std::uint32_t Select(const std::uint32_t task,
                     const std::uint32_t record,
                     const std::uint32_t requested,
                     const AiPipelineKind kind,
                     const std::uint32_t state_offset,
                     const std::uint32_t mode,
                     const std::string_view source,
                     IAiSkillActionRuntime& runtime) {
    const auto skill = kind == AiPipelineKind::Property
                           ? runtime.ReadWord(record) : record;
    const auto state = runtime.ReadWord(skill + state_offset);
    const auto category = runtime.TargetCategory(skill, kind);
    const auto target = runtime.SelectTarget(
        task, category, state, requested, mode);
    if (target == 0u) return 0u;
    runtime.LogAction(task, source, "selectTarget:%s",
                      runtime.TargetName(target));
    return runtime.CreateAction(kind, skill,
                                runtime.ReadWord(task + 4u), target);
}

}  // namespace

std::uint32_t AiSkillAction::SelectStunt(
    const std::uint32_t task, const std::uint32_t skill,
    const std::uint32_t requested,
    IAiSkillActionRuntime& runtime) {
    return Select(task, skill, requested, AiPipelineKind::Stunt,
                  4u, 2u, "selectStuntUsedPipeline", runtime);
}

std::uint32_t AiSkillAction::SelectMagic(
    const std::uint32_t task, const std::uint32_t skill,
    const std::uint32_t requested,
    IAiSkillActionRuntime& runtime) {
    return Select(task, skill, requested, AiPipelineKind::Magic,
                  8u, 3u, "selectMagicUsedPipeline", runtime);
}

std::uint32_t AiSkillAction::SelectProperty(
    const std::uint32_t task, const std::uint32_t property_record,
    const std::uint32_t requested,
    IAiSkillActionRuntime& runtime) {
    return Select(task, property_record, requested,
                  AiPipelineKind::Property, 8u, 4u,
                  "selectPropertyUsedPipeline", runtime);
}

}  // namespace pal4
