#include "pal4/cast_magic_sequence_task.h"

namespace pal4 {
std::uint32_t CastMagicSequenceTask::CreateNext(
    const std::uint32_t task, const std::uint32_t source,
    ICastMagicTransitionRuntime& runtime) {
    if (!runtime.IsActorReady(source)) return 0u;

    auto target = runtime.ReadWord(task + 48u);
    if (!runtime.IsActorReady(target))
        target = CombatReadyTargetScan::FirstReadyOther(
            runtime, target, 464u, source);
    if (target == 0u) return 0u;

    const auto magic = runtime.ReadWord(task + 52u);
    if (runtime.CanCastMagic(source, magic) &&
        runtime.ActorPropertySlot8(source, 3u))
        return runtime.MagicAction(runtime.ReadWord(task + 52u), source,
                                   target);

    if (source == target || runtime.ReadWord(source + 0x324u) ==
                                runtime.ReadWord(target + 0x324u)) {
        target = CombatReadyTargetScan::FirstReadyOther(
            runtime, runtime.ReadWord(task + 44u), 472u, source);
    }
    if (target == 0u) return 0u;
    return runtime.NormalAction(source, target, 0u);
}

std::uint32_t CastMagicSequenceTask::Complete(
    const std::uint32_t task, ICastMagicCompletionRuntime& runtime) {
    if (!runtime.CombatActive()) return 0u;
    const auto source = runtime.ReadWord(task + 44u);
    if (!runtime.SourceStateThree(source))
        return runtime.EndAction(runtime.ReadWord(task + 44u));
    runtime.NotifyDefeatedSource(runtime.ReadWord(task + 44u));
    const auto target = runtime.ReadWord(task + 48u);
    const auto current_source = runtime.ReadWord(task + 44u);
    return runtime.ProcessDead(current_source, target);
}

}  // namespace pal4
