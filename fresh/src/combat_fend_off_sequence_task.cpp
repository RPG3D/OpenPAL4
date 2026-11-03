#include "pal4/combat_fend_off_sequence_task.h"

#include "pal4/script_task_base.h"
#include "pal4/win_action_task.h"

namespace pal4 {

FendOffImpactChildFactorySlot* FendOffImpactChildFactory::Initialize(
    FendOffImpactChildFactorySlot* const slot) {
    slot->vtable = kVtable;
    return slot;
}

std::uint32_t FendOffSequenceTask::Start(
    const std::uint32_t task, IFendOffStartRuntime& runtime) {
    constexpr std::array<std::uint32_t, 3> timing{1u, 0u, 0u};
    constexpr std::uint32_t empty_text = 0x841964u;

    if (runtime.ActorCheckStateThree(runtime.ReadWord(task + 48u)))
        runtime.SetActorState(runtime.ReadWord(task + 48u), 3u, 1u);

    const auto impact_configured = runtime.ReadWord(0x8f96a8u);
    const auto impact_text = impact_configured == 0u
        ? empty_text : impact_configured;
    FendOffImpactChildFactorySlot impact_factory;
    (void)FendOffImpactChildFactory::Initialize(&impact_factory);
    const auto impact = runtime.CreateChildTask(impact_factory.vtable);
    if (impact == 0u) return 0u;
    (void)runtime.AddTaskAndInitialize(task + 4u, impact,
                                        impact_text, timing);
    const auto impact_target = runtime.ReadWord(task + 48u);
    const auto impact_source = runtime.ReadWord(task + 44u);
    runtime.WriteWord(impact + 60u, impact_source);
    runtime.WriteWord(impact + 64u, impact_target);

    runtime.ActorVirtual396(runtime.ReadWord(task + 48u), 0u);
    const auto message_configured = runtime.ReadWord(0x8f95d0u);
    const auto message_text = message_configured == 0u
        ? empty_text : message_configured;
    WinActionChildFactorySlot message_factory;
    (void)WinActionThirdChildFactory::Initialize(&message_factory);
    const auto message = runtime.CreateChildTask(message_factory.vtable);
    if (message == 0u) return 0u;
    (void)runtime.AddTaskAndInitialize(task + 4u, message,
                                        message_text, timing);
    runtime.WriteWord(message + 60u, runtime.ReadWord(task + 48u));
    runtime.AssignText(message + 64u, 0x8fa32cu, 0u, 0xffffffffu);

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

std::uint32_t FendOffSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t FendOffSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

std::uint32_t QuickFendOffSequenceTask::DestroyBase(
    const std::uint32_t task, IScriptTaskBaseRuntime& runtime) {
    return ScriptTaskBase::Destroy(task, runtime);
}

std::uint32_t QuickFendOffSequenceTask::Destroy(
    const std::uint32_t task, const std::uint8_t flags,
    IScriptTaskBaseRuntime& runtime) {
    (void)DestroyBase(task, runtime);
    if ((flags & 1u) != 0u) runtime.Free(task);
    return task;
}

}  // namespace pal4
