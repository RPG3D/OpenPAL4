#include "pal4/physical_attack_task.h"

namespace pal4 {

std::uint32_t PhysicalAttackTask::Create(
    const std::uint32_t parent, IPhysicalAttackTaskRuntime& runtime) {
    const auto task = runtime.Allocate(kSize);
    if (task == 0u) return 0u;
    struct Guard {
        IPhysicalAttackTaskRuntime& runtime;
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
    const auto mode = runtime.ReadByte(parent + 54u);
    const auto source = runtime.ReadWord(parent + 44u);
    const auto flag_52 = runtime.ReadByte(parent + 52u);
    const auto target = runtime.ReadWord(parent + 48u);
    runtime.InitializeContainer(task + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(task + 20u, mode);
    runtime.WriteWord(task + 24u, 0u);
    runtime.WriteWord(task + 28u, 0u);
    runtime.WriteWord(task + 32u, 0u);
    guard.name_initialized = true;
    runtime.AssignName(task + 20u, "PhysicalAttack");
    runtime.WriteWord(task + 48u, target);
    runtime.WriteWord(task + 44u, source);
    runtime.WriteWord(task + 36u, 0u);
    runtime.WriteByte(task + 54u, mode);
    runtime.WriteByte(task + 40u, 1u);
    runtime.WriteByte(task + 52u, flag_52);
    runtime.WriteByte(task + 53u, flag_53);
    runtime.WriteWord(task, kVtable);
    guard.task = 0u;
    return task;
}

std::uint32_t PhysicalAttackTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

}  // namespace pal4
