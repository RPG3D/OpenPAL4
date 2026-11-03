#include "pal4/script_task_base.h"

namespace pal4 {

std::uint32_t ScriptTaskBase::Destroy(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    runtime.WriteWord(task, kVtable);
    runtime.CleanupObjectCollection(task + 4u);
    const auto text = runtime.ReadWord(task + 24u);
    if (text != 0u) {
        const auto count = runtime.ReadByte(text - 1u);
        if (count == 0u || count == 0xffu)
            runtime.Free(text - 1u);
        else
            runtime.WriteByte(text - 1u,
                              static_cast<std::uint8_t>(count - 1u));
    }
    runtime.WriteWord(task + 24u, 0u);
    runtime.WriteWord(task + 28u, 0u);
    runtime.WriteWord(task + 32u, 0u);
    return runtime.DestroyContainer(task + 4u);
}

std::uint32_t ScriptTaskBase::ReturnZero(
    const std::uint32_t /*argument*/) {
    return 0u;
}

std::uint32_t ScriptTaskBase::GetTarget(
    const std::uint32_t task, IScriptTaskReadRuntime& runtime) {
    return runtime.ReadWord(task + 48u);
}

std::uint32_t ScriptTaskBase::GetActor(
    const std::uint32_t task, IScriptTaskReadRuntime& runtime) {
    return runtime.ReadWord(task + 44u);
}

std::uint32_t ScriptTaskBase::GetActorStatus(
    const std::uint32_t task,
    ICombatTaskActorStatusRuntime& runtime) {
    return runtime.ActorStatusVirtual480(runtime.ReadWord(task + 44u));
}

}  // namespace pal4
