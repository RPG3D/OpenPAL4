#include "pal4/stunt_task.h"

namespace pal4 {

std::uint8_t StuntTask::Process(
    const std::uint32_t task, IStuntTaskRuntime& runtime) {
    (void)SetTarget(task, runtime);
    (void)HandleActor(task, runtime);
    return 1u;
}

std::uint32_t StuntTask::SetTarget(
    const std::uint32_t task, IStuntTaskRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 60u);
    const auto stunt = runtime.ReadWord(task + 68u);
    const auto target = runtime.ReadWord(task + 64u);
    runtime.SetStuntTarget(source, stunt, target);
    const auto cost = runtime.ReadWord(runtime.ReadWord(task + 68u) + 116u);
    runtime.AdjustStuntCost(runtime.ReadWord(task + 60u), 8u,
        static_cast<std::int32_t>(0u - cost));
    return ApplyEffects(task, runtime);
}

std::uint32_t StuntTask::ApplyEffects(
    const std::uint32_t task, IStuntTaskRuntime& runtime) {
    const auto stunt = runtime.ReadWord(task + 68u);
    const auto amount = static_cast<std::int32_t>(
        runtime.ReadWord(stunt + 120u));
    if (amount > 0)
        runtime.AdjustActorValue(runtime.ReadWord(task + 60u),
            static_cast<std::int32_t>(0u - static_cast<std::uint32_t>(amount)));

    std::uint32_t result = stunt;
    for (std::uint32_t offset = 92u; offset < 112u; offset += 4u) {
        result = runtime.ReadWord(task + 68u);
        const auto type = static_cast<std::int32_t>(
            runtime.ReadWord(result + offset - 20u));
        if (type > 0) {
            const auto value = runtime.ReadWord(result + offset);
            result = runtime.ApplyActorEffect(
                runtime.ReadWord(task + 60u),
                static_cast<std::uint32_t>(type),
                static_cast<std::int32_t>(0u - value));
        }
    }
    return result;
}

std::uint8_t StuntTask::HandleActor(
    const std::uint32_t task, IStuntTaskRuntime& runtime) {
    auto result = runtime.ReadWord(task + 60u);
    if (!result) return static_cast<std::uint8_t>(result);
    result = runtime.ReadWord(task + 64u);
    if (!result) return static_cast<std::uint8_t>(result);
    result = runtime.ReadWord(task + 68u);
    if (!result) return static_cast<std::uint8_t>(result);

    const auto kind = runtime.StuntTargetKind(result);
    if (kind != 3u && kind != 4u) {
        runtime.WriteFloat(task + 72u, 1.0f);
        return HandleActorInternal(task, runtime.ReadWord(task + 64u), runtime);
    }

    runtime.WriteFloat(task + 72u, runtime.GroupDamageScale());
    const auto iterator = runtime.TargetIterator(runtime.ReadWord(task + 64u));
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto candidate = runtime.IteratorCurrent(iterator);
            if (runtime.CandidateEligible(candidate))
                (void)HandleActorInternal(task, candidate, runtime);
            runtime.IteratorNext(iterator);
        }
    } catch (...) {
        (void)runtime.DestroyIterator(iterator);
        throw;
    }
    return static_cast<std::uint8_t>(runtime.DestroyIterator(iterator));
}

std::uint8_t StuntTask::HandleActorInternal(
    const std::uint32_t task, const std::uint32_t target,
    IStuntTaskRuntime& runtime) {
    auto result = runtime.ActorProperty(target, 2u);
    if (!static_cast<std::uint8_t>(result))
        return static_cast<std::uint8_t>(result);
    result = runtime.ActorReady(target);
    if (!static_cast<std::uint8_t>(result))
        return static_cast<std::uint8_t>(result);
    const auto stunt = runtime.ReadWord(task + 68u);
    result = runtime.StuntIncludesSelf(stunt);
    const auto source = runtime.ReadWord(task + 60u);
    if (static_cast<std::uint8_t>(result) && target == source)
        return static_cast<std::uint8_t>(result);

    runtime.ApplyTargetHit(target, source, stunt);
    runtime.NotifySource(runtime.ReadWord(task + 60u),
                         runtime.ReadWord(runtime.ReadWord(task + 68u) + 112u),
                         target);
    result = runtime.StuntTargetKind(runtime.ReadWord(task + 68u));
    if (result == 2u || result == 4u)
        result = runtime.SetTargetState12(target,
                                          runtime.ReadWord(task + 60u));
    else if (target != runtime.ReadWord(task + 60u))
        result = runtime.SetTargetStateOne(target);
    return static_cast<std::uint8_t>(result);
}

}  // namespace pal4
