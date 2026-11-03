#include "pal4/combat_miss_sequence_task.h"

#include "pal4/script_task_base.h"
#include "pal4/win_action_task.h"

namespace pal4 {

MissMessageChildFactorySlot* MissMessageChildFactory::Initialize(
    MissMessageChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint32_t MissSequenceTask::Start(
    const std::uint32_t task, IMissStartRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    constexpr std::uint32_t empty_text = 0x841964u;

    const auto source = runtime.ReadWord(task + 44u);
    runtime.ActorVirtual512(source, runtime.ReadWord(task + 48u));

    const auto message_configured = runtime.ReadWord(0x8f960cu);
    const auto message_text = message_configured == 0u
        ? empty_text : message_configured;
    MissMessageChildFactorySlot message_factory;
    (void)MissMessageChildFactory::Initialize(&message_factory);
    const auto message = runtime.CreateChildTask(message_factory.vtable);
    if (message == 0u) return 0u;
    (void)runtime.AddTaskAndInitialize(task + 4u, message,
                                        message_text, timing);
    runtime.WriteWord(message + 60u, runtime.ReadWord(task + 48u));
    runtime.AssignText(message + 64u, 0x8fa308u, 0u, 0xffffffffu);

    const auto finish_configured = runtime.ReadWord(0x8f9770u);
    const auto finish_text = finish_configured == 0u
        ? empty_text : finish_configured;
    WinActionChildFactorySlot finish_factory;
    (void)WinActionSecondChildFactory::Initialize(&finish_factory);
    const auto finish = runtime.CreateChildTask(finish_factory.vtable);
    if (finish == 0u) return 0u;
    const auto result = runtime.AddTaskAndInitialize(
        task + 4u, finish, finish_text, timing);
    runtime.WriteWord(finish + 60u, runtime.ReadWord(task + 48u));
    return result;
}

std::uint32_t MissSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t MissSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

std::uint32_t QuickMissSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickMissSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

}  // namespace pal4
