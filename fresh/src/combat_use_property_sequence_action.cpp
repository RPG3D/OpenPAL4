#include "pal4/combat_use_property_sequence_task.h"

namespace pal4 {

std::uint32_t UsePropertySequenceTask::CreateAction(
    const std::uint32_t task, const std::uint32_t source,
    IUsePropertyActionRuntime& runtime) {
    if (!runtime.IsActorReady(source)) return 0u;

    auto target = runtime.ReadWord(task + 48u);
    if (!runtime.IsActorReady(target))
        target = CombatReadyTargetScan::FirstReadyOther(
            runtime, target, 464u, source);
    if (target == 0u) return 0u;

    const auto property = runtime.ReadWord(task + 52u);
    const auto first = runtime.ReadWord(property);
    const auto second = runtime.ReadWord(property + 4u);
    if (runtime.PropertyExists(first, second) &&
        runtime.ActorPropertySlot8(source, 4u))
        return runtime.UsePropertyAction(
            runtime.ReadWord(task + 52u), source, target);

    if (source == target || runtime.ReadWord(source + 804u) ==
                                runtime.ReadWord(target + 804u))
        target = CombatReadyTargetScan::FirstReadyOther(
            runtime, runtime.ReadWord(task + 44u), 472u, source);
    if (target == 0u) return 0u;
    return runtime.NormalAction(source, target, 0u);
}

}  // namespace pal4
