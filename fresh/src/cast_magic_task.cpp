#include "pal4/cast_magic_task.h"

namespace pal4 {

std::uint8_t CastMagicTask::Process(const std::uint32_t task,
                                    ICastMagicTaskRuntime& runtime) {
    (void)SetTarget(task, runtime);
    (void)HandleActor(task, 0, runtime);
    return 1u;
}

int CastMagicTask::SetTarget(const std::uint32_t task,
                             ICastMagicTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    const auto magic = runtime.ReadWord(task + 68u);
    runtime.SetActorTarget(actor, magic, runtime.ReadWord(task + 64u));
    const auto current_magic = runtime.ReadWord(task + 68u);
    const auto cost = runtime.ReadWord(current_magic + 76u);
    return runtime.ChangeActorValue(runtime.ReadWord(task + 60u), 9u,
        static_cast<std::int32_t>(0u - cost));
}

int CastMagicTask::HandleActor(const std::uint32_t task, const int mode,
                               ICastMagicTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (!actor) return 0;
    const auto target = runtime.ReadWord(task + 64u);
    if (!target) return 0;
    const auto magic = runtime.ReadWord(task + 68u);
    if (!magic) return 0;

    const auto magic_mode = runtime.MagicMode(magic);
    if (magic_mode != 3u && magic_mode != 4u) {
        runtime.WriteScale(task + 72u, 1.0f);
        return HandleActorInternal(task, target, mode, runtime);
    }

    runtime.WriteScale(task + 72u, runtime.DamageScale(26u));
    const auto iterator = runtime.TargetIterator(target);
    if (!iterator) return 0;
    struct IteratorGuard final {
        ICastMagicTaskRuntime& runtime;
        std::uint32_t iterator;
        bool active = true;
        ~IteratorGuard() {
            if (active) (void)runtime.ReleaseIterator(iterator);
        }
    } guard{runtime, iterator};
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (runtime.ActorEligible(candidate))
            (void)HandleActorInternal(task, candidate, mode, runtime);
        runtime.IteratorNext(iterator);
    }
    guard.active = false;
    return runtime.ReleaseIterator(iterator);
}

int CastMagicTask::HandleActorInternal(
    const std::uint32_t task, const std::uint32_t target, const int,
    ICastMagicTaskRuntime& runtime) {
    int result = runtime.ActorProperty3(target);
    if (static_cast<std::uint8_t>(result) == 0u) return result;
    const auto magic = runtime.ReadWord(task + 68u);
    result = runtime.MagicRejectsSelf(magic);
    const auto attacker = runtime.ReadWord(task + 60u);
    if (static_cast<std::uint8_t>(result) != 0u && target == attacker)
        return result;

    if (runtime.ReadWord(attacker + 804u) !=
            runtime.ReadWord(target + 804u) &&
        !runtime.CheckDodge(attacker + 340u, target + 340u,
                            runtime.ReadFloat(magic + 84u)))
        return runtime.CallActor(attacker, 512u, target, 0u);

    const auto current_attacker = runtime.ReadWord(task + 60u);
    const auto current_magic = runtime.ReadWord(task + 68u);
    (void)runtime.CallActor(target, 488u, current_attacker, current_magic);
    const auto event_magic = runtime.ReadWord(task + 68u);
    const auto event = runtime.ReadWord(event_magic + 72u);
    (void)runtime.CallActor(runtime.ReadWord(task + 60u), 264u, event, target);
    (void)NotifyCastProperties(task, target, runtime);
    result = static_cast<int>(runtime.MagicMode(runtime.ReadWord(task + 68u)));
    if (result == 2 || result == 4)
        return runtime.CallActor(target, 260u, 12u,
                                 runtime.ReadWord(task + 60u));
    if (target != runtime.ReadWord(task + 60u))
        return runtime.CallActor(target, 396u, 1u, 0u);
    return result;
}

int CastMagicTask::NotifyCastProperties(
    const std::uint32_t task, const std::uint32_t target,
    ICastMagicTaskRuntime& runtime) {
    (void)runtime.CallActor(target, 260u, 0u, target);
    const auto magic = runtime.ReadWord(task + 68u);
    const auto category = runtime.ReadWord(magic + 4u);
    if (category > 4u) return static_cast<int>(category);
    return runtime.CallActor(target, 260u, 7u + category, target);
}

}  // namespace pal4
