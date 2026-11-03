#include "pal4/combat_process_message_task.h"

#include "pal4/combat_action_task_operations.h"

namespace pal4 {
namespace {

void ReleaseString(const std::uint32_t record,
                   ICombatProcessMessageTaskRuntime& runtime) {
    const auto data = runtime.ReadWord(record);
    if (data) {
        const auto refs = runtime.ReadByte(data - 1u);
        if (refs == 0u || refs == 0xffu)
            runtime.Free(data - 1u);
        else
            runtime.WriteByte(data - 1u,
                              static_cast<std::uint8_t>(refs - 1u));
    }
    runtime.WriteWord(record, 0u);
    runtime.WriteWord(record + 4u, 0u);
    runtime.WriteWord(record + 8u, 0u);
}

}  // namespace

std::uint32_t CombatProcessMessageTask::Destruct(
    const std::uint32_t task, ICombatProcessMessageTaskRuntime& runtime) {
    runtime.WriteWord(task, kVtable);
    CombatProcessMessageSubscriptions::Clear(task, runtime);
    for (std::uint32_t index = 0u; index < 5u; ++index)
        runtime.ResetEffect(task + 96u + 24u * index);
    for (std::uint32_t index = 5u; index > 0u; --index)
        DestroyEffectSlot(task + 96u + 24u * (index - 1u), runtime);
    ReleaseString(task + 68u, runtime);
    return runtime.BaseDestruct(task);
}

std::uint32_t CombatProcessMessageTask::ResetTemporaryEffect(
    const std::uint32_t first, const std::uint32_t second,
    const std::uint32_t third, ICombatProcessMessageTaskRuntime& runtime) {
    return runtime.ResetTemporaryEffect(first, second, third);
}

void CombatProcessMessageTask::DestroyEffectSlot(
    const std::uint32_t slot, ICombatProcessMessageTaskRuntime& runtime) {
    ReleaseString(slot + 12u, runtime);
    runtime.ResetEffect(slot);
}

std::uint32_t CombatProcessMessageTask::HandleAnimationMessage(
    const std::uint32_t task, const std::uint32_t event,
    ICombatProcessMessageDispatchRuntime& runtime) {
    const auto actor = runtime.GetDialogData(event);
    if (actor == runtime.ReadWord(task + 60u))
        return runtime.DispatchAnimationMessage(
            task, runtime.ReadWord(event + 4u));
    return actor;
}

std::uint32_t CombatProcessMessageTask::HandleEffectMessage(
    const std::uint32_t task, const std::uint32_t event,
    ICombatProcessMessageDispatchRuntime& runtime) {
    return runtime.DispatchEffectMessage(task, runtime.ReadWord(event),
        runtime.ReadWord(event + 4u), event + 8u);
}

std::uint8_t CombatProcessMessageTask::StartAction(
    const std::uint32_t task, ICombatProcessMessageFlowRuntime& runtime) {
    runtime.RegisterSubscriptions(task);
    return runtime.ProcessAction(task);
}

std::uint8_t CombatProcessMessageTask::StopAction(
    const std::uint32_t task, const std::uint32_t context,
    ICombatProcessMessageFlowRuntime& runtime) {
    runtime.ClearSubscriptions(task);
    for (std::uint32_t index = 0; index < 5u; ++index)
        runtime.HandleEffectEvent(task + 96u + 24u * index, context, false);
    return 0;
}

bool CombatProcessMessageTask::IsActionComplete(
    const std::uint32_t task, ICombatProcessMessageFlowRuntime& runtime) {
    const bool animation_complete = runtime.IsAnimationComplete(task);
    for (std::uint32_t index = 0; index < 5u; ++index) {
        if (runtime.EffectSlotActive(task + 96u + 24u * index))
            return false;
    }
    return animation_complete;
}

std::uint32_t CombatProcessMessageTask::CallGameObjectCommand(
    const std::uint32_t first, const std::uint32_t second,
    const std::uint32_t actor, const std::uint32_t fourth,
    ICombatProcessMessageFlowRuntime& runtime) {
    runtime.CallActorCommand(actor, first, second, fourth);
    return first;
}

}  // namespace pal4
