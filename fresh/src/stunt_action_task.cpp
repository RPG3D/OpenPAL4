#include "pal4/stunt_action_task.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {

std::uint32_t StuntActionHandlers::Subscribe(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    Clear(task, runtime);
    std::uint32_t result = 0u;
    for (std::uint32_t index = 0u; index < kBindings.size(); ++index) {
        const auto& binding = kBindings[index];
        const auto key = runtime.ReadWord(binding.key_address);
        result = runtime.Register(runtime.EventRouter(), 2u, key,
                                  task, binding.callback);
        runtime.WriteWord(task + 216u + 8u * index, result);
        runtime.WriteWord(task + 220u + 8u * index, key);
    }
    return result;
}

void StuntActionHandlers::Clear(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(task, 216u, 5u, runtime);
}

void StuntActionTask::Destroy(
    const std::uint32_t task, IStuntActionTaskRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    runtime.WriteWord(task, kVtable);
    StuntActionHandlers::Clear(task, events);
    runtime.DestroyEmbedded(task + 256u);
    runtime.DestroyBase(task);
}

std::uint8_t StuntActionTask::Process(
    const std::uint32_t task, IStuntActionTaskRuntime& runtime,
    IEventSubscriptionRuntime& events, IStuntTaskRuntime& stunt) {
    (void)StuntActionHandlers::Subscribe(task, events);
    runtime.StartAction(task);
    (void)StuntTask::SetTarget(task + 256u, stunt);
    return 1u;
}

std::uint8_t StuntActionTask::Stop(
    const std::uint32_t task, const std::uint32_t frame_context,
    IStuntActionTaskRuntime& runtime, IEventSubscriptionRuntime& events) {
    StuntActionHandlers::Clear(task, events);
    runtime.ActorReset(runtime.ReadWord(task + 60u));
    return runtime.StopAction(task, frame_context);
}

bool StuntActionTask::CheckStatus(
    const std::uint32_t task, IStuntActionFrameRuntime& runtime) {
    return runtime.IsActionPending(task) &&
           runtime.ReadByte(task + 332u) != 0u;
}

std::uint8_t StuntActionTask::UpdateActorState(
    const std::uint32_t task, IStuntActionFrameRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    const auto current = runtime.ActorAnimationName(actor);
    const auto length = runtime.ReadWord(task + 72u);
    if (runtime.TaskAnimationName(task + 68u, length) == current &&
        runtime.ActorAnimationFlag(runtime.ReadWord(task + 60u)))
        runtime.ActorSetState(runtime.ReadWord(task + 60u), 0u);
    return 1u;
}

std::uint32_t StuntActionTask::GetActorProperty(
    const std::uint32_t task, ICombatActionTargetRuntime& runtime) {
    return runtime.ReadWord(task + 320u);
}

int StuntActionTask::ProcessCommand(
    const std::uint32_t task, const std::string_view message_name,
    IStuntActionTaskRuntime& runtime,
    IStuntActionProcessRuntime& process,
    IStuntTaskRuntime& stunt) {
    std::string command(message_name);
    std::transform(command.begin(), command.end(), command.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    try {
        if (command.size() > 2u && command.compare(0u, 3u, "atk") == 0)
            ProcessInternal(task, process, stunt);
        else
            runtime.ProcessAnimationCommand(task, message_name);
    } catch (...) {
        (void)runtime.CleanupCommand(command);
        throw;
    }
    return runtime.CleanupCommand(command);
}

}  // namespace pal4
