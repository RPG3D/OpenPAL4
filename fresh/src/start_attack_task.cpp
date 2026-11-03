#include "pal4/start_attack_task.h"

namespace pal4 {

std::uint32_t StartAttackTask::Create(
    const std::uint32_t parent, IStartAttackTaskRuntime& runtime) {
    const auto selected = runtime.ReadWord(parent + 52u);
    const auto task = runtime.Allocate(kSize);
    if (task == 0u) return 0u;
    struct Guard {
        IStartAttackTaskRuntime& runtime;
        std::uint32_t task;
        bool container_initialized = false;
        bool name_allocated = false;
        ~Guard() {
            if (task == 0u) return;
            if (name_allocated) runtime.CleanupName(task + 20u);
            if (container_initialized) runtime.DestroyContainer(task + 4u);
            runtime.Free(task);
        }
    } guard{runtime, task};

    std::uint32_t source;
    std::uint32_t target;
    std::uint8_t mode;
    if (selected != 0u) {
        target = selected;
        mode = runtime.ReadByte(parent + 56u);
        source = runtime.ReadWord(parent + 44u);
    } else {
        source = runtime.ReadWord(parent + 44u);
        mode = runtime.ReadByte(parent + 56u);
        target = runtime.ReadWord(parent + 48u);
    }
    runtime.InitializeContainer(task + 4u);
    guard.container_initialized = true;
    runtime.WriteByte(task + 20u, mode);
    if (selected != 0u) {
        runtime.SetNameErrorFlag(task + 20u, 0u);
    } else {
        runtime.WriteWord(task + 24u, 0u);
        runtime.WriteWord(task + 28u, 0u);
        runtime.WriteWord(task + 32u, 0u);
    }
    constexpr std::string_view name = "StartAttack";
    if (runtime.AllocateNameBuffer(task + 20u,
                                   static_cast<std::uint32_t>(name.size()),
                                   1u)) {
        guard.name_allocated = true;
        runtime.CopyName(task + 20u, name);
        runtime.WriteWord(task + 28u,
                          static_cast<std::uint32_t>(name.size()));
    }
    runtime.WriteWord(task + 36u, 0u);
    runtime.WriteByte(task + 40u, 1u);
    runtime.WriteWord(task + 44u, source);
    runtime.WriteWord(task + 48u, target);
    runtime.WriteByte(task + 52u, selected == 0u ? 1u : 0u);
    runtime.WriteByte(task + 53u, selected == 0u ? 0u : 1u);
    runtime.WriteByte(task + 54u, mode);
    runtime.WriteWord(task, kVtable);
    guard.task = 0u;
    return task;
}

std::uint32_t StartAttackTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

}  // namespace pal4
