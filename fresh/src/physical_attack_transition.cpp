#include "pal4/physical_attack_task.h"

namespace pal4 {
std::uint32_t PhysicalAttackTask::BuildContinuation(
    const std::uint32_t parent, const bool repeat,
    IPhysicalAttackContinuationRuntime& runtime) {
    const auto task = runtime.Allocate(56u);
    if (task == 0u) return 0u;
    struct Guard {
        IPhysicalAttackContinuationRuntime& runtime;
        std::uint32_t task;
        bool container_initialized = false;
        bool name_initialized = false;
        ~Guard() {
            if (task == 0u) return;
            if (name_initialized) runtime.CleanupName(task + 20u);
            if (container_initialized) runtime.DestroyContainer(task + 4u);
            runtime.Free(task);
        }
    } guard{runtime, task};

    const auto flag_53 = runtime.ReadByte(parent + 53u);
    std::uint8_t mode = 0u;
    if (repeat) mode = runtime.ReadByte(parent + 54u);
    const auto source = repeat
        ? runtime.ReadWord(parent + 44u)
        : 0u;
    const auto flag_52 = runtime.ReadByte(parent + 52u);
    const auto actual_source = repeat
        ? source : runtime.ReadWord(parent + 44u);
    const auto target = runtime.ReadWord(parent + 48u);
    runtime.InitializeContainer(task + 4u);
    guard.container_initialized = true;

    const auto name = repeat
        ? std::string_view("PhysicalAttack")
        : std::string_view("EndAttack");
    runtime.WriteByte(task + 20u, repeat ? mode : flag_52);
    if (repeat) {
        runtime.SetNameErrorFlag(task + 20u, 0u);
    } else {
        runtime.WriteWord(task + 24u, 0u);
        runtime.WriteWord(task + 28u, 0u);
        runtime.WriteWord(task + 32u, 0u);
    }
    guard.name_initialized = true;
    if (runtime.AllocateNameBuffer(
            task + 20u, static_cast<std::uint32_t>(name.size()), 1u)) {
        runtime.CopyName(task + 20u, name);
        runtime.WriteWord(task + 28u,
                          static_cast<std::uint32_t>(name.size()));
    }
    runtime.WriteWord(task + 44u, actual_source);
    runtime.WriteWord(task + 48u, target);
    runtime.WriteWord(task + 36u, 0u);
    runtime.WriteByte(task + 40u, 1u);
    runtime.WriteByte(task + 52u, flag_52);
    runtime.WriteByte(task + 53u, flag_53);
    if (repeat) runtime.WriteByte(task + 54u, mode);
    runtime.WriteWord(task, repeat ? PhysicalAttackTask::kVtable
                                   : EndAttackTask::kVtable);
    guard.task = 0u;
    return task;
}

std::uint32_t IPhysicalAttackTransitionRuntime::CreateContinuation(std::uint32_t parent,bool repeat){
    return PhysicalAttackTask::BuildContinuation(parent,repeat,*this);
}

std::uint32_t PhysicalAttackTask::CreateNext(
    const std::uint32_t task,
    IPhysicalAttackTransitionRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 44u);
    if (runtime.ActorStateThree(source)) {
        runtime.NotifyStateThree(source);
        return runtime.ProcessWounded(source,
                                      runtime.ReadWord(task + 48u));
    }
    const auto count = runtime.ActorCounter(source, 3u);
    if (count > 0)
        runtime.SetActorCounter(source, 3u, count - 1);
    return runtime.CreateContinuation(task, count > 0);
}

}  // namespace pal4
