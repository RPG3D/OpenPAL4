#include "pal4/ai_action_task.h"

namespace pal4 {

bool AiActionTargetCatalog::CompareTargets(
    const AiActionTargetKey& key, const std::uint32_t record,
    IAiActionTargetCatalogRuntime& runtime) {
    if (runtime.ReadWord(record + 4u) != key.animation_id)
        return false;
    if (runtime.ReadWord(record + 8u) != key.selection_id)
        return false;
    return runtime.ReadWord(record + 12u) == key.target_id;
}

std::uint32_t AiActionTargetCatalog::ChooseRecord(
    const std::uint32_t task, IAiActionTargetCatalogRuntime& runtime) {
    const auto animation = runtime.PlayAnimation(task);
    if (animation == 0u) return 0u;
    const auto selection_id = runtime.ProcessTargetSelection(task, animation);
    const auto target_id = runtime.SelectTarget(task, animation);
    const AiActionTargetKey key{
        runtime.AnimationId(task), target_id, selection_id};
    const auto base = runtime.RecordBase();
    const auto count = runtime.RecordCount();
    for (std::uint32_t index = 0u; base != 0u && index < count;
         ++index) {
        const auto record = base + index * kRecordSize;
        if (CompareTargets(key, record, runtime)) return record;
    }
    runtime.ReportMissing(animation);
    return 0u;
}

}  // namespace pal4
