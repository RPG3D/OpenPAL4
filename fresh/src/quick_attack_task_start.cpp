#include "pal4/quick_attack_task.h"

namespace pal4 {

std::uint32_t QuickAttackTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickAttackEndTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickAttackEndTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

QuickAttackChildFactorySlot* QuickAttackChildFactory::Initialize(
    QuickAttackChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

bool QuickAttackTask::Start(const std::uint32_t task,
                            IQuickAttackStartRuntime& runtime) {
    const auto source = runtime.ReadWord(task + 44u);
    const auto target = runtime.ReadWord(task + 48u);
    const auto selected = ScriptActorEventProcessor::SelectTarget(
        source, target, runtime);
    if (selected != 0u) {
        runtime.WriteWord(task + 44u, selected);
        runtime.SetSelectedTarget(selected, selected, target);
    }

    runtime.NotifySource(runtime.ReadWord(task + 44u));
    const auto configured_text = runtime.ReadWord(0x8f9694u);
    const auto source_text = configured_text == 0u ? 0x841964u
                                                     : configured_text;
    QuickAttackChildFactorySlot factory;
    (void)QuickAttackChildFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory);
    if (child == 0u) return false;
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    runtime.AddTaskAndInitialize(task + 4u, child, source_text, timing);
    runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    runtime.WriteWord(child + 64u, runtime.ReadWord(task + 48u));
    runtime.WriteByte(child + 72u, selected != 0u ? 1u : 0u);
    runtime.WriteByte(child + 73u, 0u);
    return selected != 0u;
}

}  // namespace pal4
