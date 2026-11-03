#include "pal4/ai_action_task.h"

namespace pal4 {

std::uint32_t AiActionAnimation::GetAnimationId(
    const std::uint32_t task, IAiActionAnimationRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 4u);
    if (actor != 0u) return runtime.ActorAnimationId(actor);
    runtime.ReportMissingActor();
    return 0xffffffffu;
}

std::uint32_t AiActionAnimation::FindRecord(
    const std::uint32_t task, IAiActionAnimationRuntime& runtime) {
    if (runtime.ReadWord(task + 4u) == 0u) {
        runtime.ReportMissingActor();
        return 0u;
    }
    const auto id = GetAnimationId(task, runtime);
    const auto record = runtime.FindAnimationRecord(id);
    if (record != 0u) return record;
    runtime.BeginMissingAnimation();
    runtime.ReportMissingAnimation(GetAnimationId(task, runtime));
    return 0u;
}

}  // namespace pal4
