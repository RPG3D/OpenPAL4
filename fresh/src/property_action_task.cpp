#include "pal4/property_action_task.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {

std::uint32_t PropertyActionHandlers::Subscribe(
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

void PropertyActionHandlers::Clear(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(task, 216u, 2u, runtime);
}

void PropertyActionTask::Destroy(
    const std::uint32_t task, IPropertyActionTaskRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    runtime.WriteWord(task, kVtable);
    PropertyActionHandlers::Clear(task, events);
    runtime.ResetEffect(task + 312u);
    runtime.DestroyEmbedded(task + 240u);
    runtime.ResetEffect(task + 232u);
    runtime.DestroyBase(task);
}

void PropertyActionTask::CleanupEmbedded(
    const std::uint32_t embedded, IPropertyActionTaskRuntime& runtime) {
    runtime.ResetEffect(embedded + 72u);
    runtime.DestroyEmbedded(embedded);
}

int PropertyActionTask::ProcessCommand(
    const std::uint32_t task, const std::string_view message_name,
    IPropertyActionTaskRuntime& runtime,
    IUsePropertyTaskRuntime& property) {
    std::string command(message_name);
    std::transform(command.begin(), command.end(), command.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    try {
        if (command.size() > 2u && command.compare(0u, 3u, "atk") == 0) {
            UsePropertyTask::ProcessInternal(task + 240u, property);
            const auto iterator = runtime.TargetIterator(
                runtime.ReadWord(task + 304u));
            try {
                while (!runtime.IteratorAtEnd(iterator)) {
                    const auto candidate = runtime.IteratorCurrent(iterator);
                    if (candidate != runtime.ReadWord(task + 60u) &&
                        runtime.ActorReady(candidate))
                        runtime.ActorSetState(candidate, 1u);
                    runtime.IteratorNext(iterator);
                }
            } catch (...) {
                runtime.DestroyIterator(iterator);
                throw;
            }
            runtime.DestroyIterator(iterator);
        } else {
            runtime.ProcessAnimationCommand(task, message_name);
        }
    } catch (...) {
        (void)runtime.CleanupCommand(command);
        throw;
    }
    return runtime.CleanupCommand(command);
}

bool PropertyActionTask::CheckStatus(
    const std::uint32_t task, IPropertyActionTaskRuntime& runtime) {
    return runtime.IsActionPending(task) &&
           runtime.ReadByte(task + 320u) != 0u;
}

std::uint8_t PropertyActionTask::Process(
    const std::uint32_t task, IPropertyActionTaskRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    (void)PropertyActionHandlers::Subscribe(task, events);
    runtime.StartAction(task);
    return 1u;
}

std::uint8_t PropertyActionTask::Stop(
    const std::uint32_t task, const std::uint32_t frame_context,
    IPropertyActionTaskRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    PropertyActionHandlers::Clear(task, events);
    runtime.StopAction(task, frame_context);
    runtime.ResetEmbedded(task + 240u);
    return 1u;
}

}  // namespace pal4
