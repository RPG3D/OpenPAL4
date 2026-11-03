#include "pal4/final_hit_task.h"

namespace pal4 {

std::uint32_t FinalHitTaskEvents::Subscribe(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    Clear(task, runtime);
    const auto key = runtime.ReadWord(kEventKey);
    const auto handle = runtime.Register(runtime.EventRouter(), 2u,
                                         key, task, kCallback);
    runtime.WriteWord(task + 96u, key);
    runtime.WriteWord(task + 92u, handle);
    return handle;
}

void FinalHitTaskEvents::Clear(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(task, 92u, 1u, runtime);
}

std::uint32_t FinalHitTask::Destroy(
    const std::uint32_t task, IFinalHitTaskRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    runtime.WriteWord(task, kVtable);
    FinalHitTaskEvents::Clear(task, events);
    runtime.ResetOwnedEffect(task + 80u);
    const auto data = runtime.ReadWord(task + 64u);
    if (data) {
        const auto refs = runtime.ReadByte(data - 1u);
        if (refs == 0u || refs == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(refs - 1u));
    }
    runtime.WriteWord(task + 64u, 0u);
    runtime.WriteWord(task + 68u, 0u);
    runtime.WriteWord(task + 72u, 0u);
    return runtime.DestroyBase(task);
}

std::uint8_t FinalHitTask::Process(
    const std::uint32_t task, IFinalHitTaskRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    (void)FinalHitTaskEvents::Subscribe(task, events);
    return ProcessEffect(task, runtime);
}

std::uint8_t FinalHitTask::ProcessEffect(
    const std::uint32_t task, IFinalHitTaskRuntime& runtime) {
    const auto name = runtime.ReadText(task + 64u);
    auto effect = runtime.CreateEffect(
        runtime.CombatCamera(runtime.CombatSystem()), name,
        kEffectPosition, 1u, 0u);
    try {
        runtime.WriteWord(task + 80u, effect.first);
        runtime.WriteWord(task + 84u, effect.second);
    } catch (...) {
        runtime.ResetTemporaryEffect(effect);
        throw;
    }
    runtime.ResetTemporaryEffect(effect);

    const auto actor = runtime.ReadWord(task + 76u);
    const auto source = actor ? actor + 724u : 0u;
    const auto text = runtime.CreateText("T1");
    try {
        runtime.SetProperties(task + 80u, text, source);
    } catch (...) {
        if (text) runtime.ReleaseText(text);
        throw;
    }
    if (text) runtime.ReleaseText(text);
    runtime.UpdatePosition(
        task + 80u, runtime.ChildPosition(runtime.ReadWord(task + 76u)), 0u);
    runtime.FinalizeEffect(task + 80u);
    if (!runtime.NeedsUpdate(task + 80u)) runtime.WriteByte(task + 88u, 1u);
    return 1u;
}

std::uint8_t FinalHitTask::Stop(
    const std::uint32_t task, IEventSubscriptionRuntime& events) {
    FinalHitTaskEvents::Clear(task, events);
    return 1u;
}

std::uint8_t FinalHitTask::CheckStatus(
    const std::uint32_t task, IFinalHitTaskRuntime& runtime) {
    return runtime.ReadByte(task + 88u);
}

std::uint32_t FinalHitTask::HandleEvent(
    const std::uint32_t task, const std::uint32_t event,
    IFinalHitTaskRuntime& runtime) {
    if (runtime.ReadWord(task + 80u) == runtime.ReadWord(event) &&
        runtime.ReadWord(task + 84u) == runtime.ReadWord(event + 4u))
        runtime.WriteByte(task + 88u, 1u);
    return event;
}

}  // namespace pal4
