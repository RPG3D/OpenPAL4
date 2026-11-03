#include "pal4/combat_defend_sequence_task.h"

#include "pal4/script_task_base.h"

namespace pal4 {

std::uint32_t DefendSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t DefendSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

DefendStatusTaskFactorySlot* DefendStatusTaskFactory::Initialize(
    DefendStatusTaskFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint32_t DefendSequenceTask::Start(
    const std::uint32_t task, IDefendSequenceRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    runtime.NotifyActor(runtime.ReadWord(task + 44u));
    const auto configured = runtime.ReadWord(0x8f96f8u);
    const auto text = configured == 0u ? 0x841964u : configured;
    DefendStatusTaskFactorySlot factory;
    (void)DefendStatusTaskFactory::Initialize(&factory);
    const auto child = runtime.CreateChildTask(factory.vtable);
    if (child == 0u) return 0u;
    const auto result = runtime.AddTaskAndInitialize(
        task + 4u, child, text, timing);
    runtime.WriteWord(child + 60u, runtime.ReadWord(task + 44u));
    return result;
}

}  // namespace pal4
