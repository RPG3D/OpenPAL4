#include "pal4/lock_control_task.h"

namespace pal4 {

std::uint32_t LockControlTask::Construct(
    const std::uint32_t object, ILockControlTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    return object;
}

std::uint8_t LockControlTask::Enter(
    const std::uint32_t object, ILockControlTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x40u) == 0u) return 0u;
    runtime.ResetActorAndSync(0u);
    runtime.ProcessUiEvents(0u);
    const auto actor = runtime.CurrentActor();
    if (actor != 0u) runtime.SetActorState(actor, "C01");
    runtime.SetPlayerState(0u);
    runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

std::uint32_t UnlockControlTask::Construct(
    const std::uint32_t object, ILockControlTaskRuntime& runtime) {
    runtime.BaseConstruct(object);
    runtime.WriteWord(object, kVtable);
    runtime.WriteByte(object + 0x3cu, 0u);
    return object;
}

std::uint8_t UnlockControlTask::Enter(
    const std::uint32_t object, ILockControlTaskRuntime& runtime) {
    if (runtime.ReadWord(object + 0x40u) == 0u) return 0u;
    runtime.ResetActorAndSync(1u);
    runtime.ProcessUiEvents(1u);
    runtime.SetPlayerState(1u);
    runtime.WriteByte(object + 0x3cu, 1u);
    return 1u;
}

}  // namespace pal4
