#include "pal4/combat_end_action_task.h"
#include "pal4/script_task_base.h"

namespace pal4 {

std::uint8_t CombatEndActionTask::Start(
    const std::uint32_t task, ICombatEndActionRuntime& runtime) {
    runtime.NotifySource(runtime.ReadWord(task + 44u));
    if (!runtime.SourceStateThree(runtime.ReadWord(task + 44u))) return 0u;
    const auto source = runtime.ReadWord(task + 44u);
    const auto action = runtime.ProcessDead(source, source);
    return runtime.RegisterActorEvent(action);
}

std::uint32_t CombatEndActionTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t CombatEndActionTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

}  // namespace pal4
