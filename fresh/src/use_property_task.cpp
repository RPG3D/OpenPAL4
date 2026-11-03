#include "pal4/use_property_task.h"

namespace pal4 {
namespace {

void ApplyCandidate(const std::uint32_t task,
                    const std::uint32_t source,
                    const std::uint32_t target,
                    const std::uint32_t property,
                    const std::uint32_t value,
                    IUsePropertyTaskRuntime& runtime) {
    runtime.ApplyToTarget(target, source, property);
    runtime.NotifySource(source, value, target);
    if (runtime.ReadWord(source + 804u) ==
        runtime.ReadWord(target + 804u)) return;

    const auto scale = runtime.ActorScale(target);
    auto effect = runtime.CreateChestEffect(
        target, "EFF14", "hook_ef_chest");
    try {
        runtime.WriteWord(task + 72u, effect.first);
        runtime.WriteWord(task + 76u, effect.second);
    } catch (...) {
        runtime.ResetTemporaryEffect(effect);
        throw;
    }
    runtime.ResetTemporaryEffect(effect);
    runtime.SetEffectScale(task + 72u, scale, 0u);
    runtime.NotifyTarget(target, source);
}

}  // namespace

std::uint32_t UsePropertyTask::SetActorsAndProperty(
    const std::uint32_t task, const std::uint32_t source,
    const std::uint32_t target, const std::uint32_t property,
    IUsePropertyTargetRuntime& runtime) {
    runtime.WriteWord(task + 60u, source);
    runtime.WriteWord(task + 64u, target);
    runtime.WriteWord(task + 68u, property);
    return property;
}

std::uint8_t UsePropertyTask::Process(
    const std::uint32_t task, IUsePropertyTaskRuntime& runtime) {
    ProcessInternal(task, runtime);
    return 1u;
}

bool UsePropertyTask::CheckStatus(
    const std::uint32_t task, IUsePropertyTaskRuntime& runtime) {
    return !runtime.NeedsUpdate(task + 72u);
}

void UsePropertyTask::ProcessInternal(
    const std::uint32_t task, IUsePropertyTaskRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 60u);
    if (!source) return;
    const auto target = runtime.ReadWord(task + 64u);
    if (!target) return;
    const auto property = runtime.ReadWord(task + 68u);
    if (!property) return;

    runtime.UseProperty(source, property, target);
    runtime.ConsumeProperty(source, runtime.ReadWord(property), -1);
    if (static_cast<std::uint8_t>(runtime.TargetProperty(target, 4u)) == 0u)
        return;
    const auto kind = runtime.PropertyKind(property);
    const auto value = runtime.ReadWord(property + 148u);
    if (kind != 3u && kind != 4u) {
        ApplyCandidate(task, source, target, property, value, runtime);
        return;
    }

    const auto iterator = runtime.TargetIterator(target);
    try {
        while (!runtime.IteratorAtEnd(iterator)) {
            const auto candidate = runtime.IteratorCurrent(iterator);
            if (runtime.CandidateEligible(candidate))
                ApplyCandidate(task, source, candidate,
                               property, value, runtime);
            runtime.IteratorNext(iterator);
        }
    } catch (...) {
        runtime.DestroyIterator(iterator);
        throw;
    }
    runtime.DestroyIterator(iterator);
}

}  // namespace pal4
