#include "pal4/combat_use_property_sequence_task.h"

#include "pal4/script_task_base.h"

namespace pal4 {

std::uint32_t UsePropertySequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t UsePropertySequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

std::uint32_t QuickUsePropertySequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickUsePropertySequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

}  // namespace pal4
