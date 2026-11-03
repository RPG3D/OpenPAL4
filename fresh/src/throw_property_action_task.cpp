#include "pal4/throw_property_action_task.h"

#include "pal4/combat_effect_action_callbacks.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {
namespace {

constexpr CombatEffectActionLayout kEffectLayout{392u, 410u, 232u};

}  // namespace

std::uint32_t ThrowPropertyActionHandlers::Subscribe(
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

void ThrowPropertyActionHandlers::Clear(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(task, 216u, 2u, runtime);
}

void ThrowPropertyActionTask::Destroy(
    const std::uint32_t task, IThrowPropertyActionRuntime& runtime,
    IEventSubscriptionRuntime& events,
    IThrowPropertyTaskRuntime& throw_runtime) {
    runtime.WriteWord(task, kVtable);
    ThrowPropertyActionHandlers::Clear(task, events);
    runtime.ResetEffect(task + 400u);
    runtime.DestroyEmbeddedProperty(task + 328u);
    ThrowPropertyTask::Destroy(task + 240u, throw_runtime);
    runtime.ResetEffect(task + 232u);
    runtime.DestroyBase(task);
}

int ThrowPropertyActionTask::ProcessCommand(
    const std::uint32_t task, const std::string_view message_name,
    IThrowPropertyActionRuntime& runtime,
    IThrowPropertyTaskRuntime& throw_runtime) {
    std::string command(message_name);
    std::transform(command.begin(), command.end(), command.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    try {
        if (command.size() >= 3u && command.compare(0u, 3u, "atk") == 0) {
            if (runtime.ReadByte(task + 409u) == 0u) {
                (void)ThrowPropertyTask::Process(task + 240u, throw_runtime);
                runtime.WriteByte(task + 409u, 1u);
            }
        } else {
            runtime.ProcessAnimationCommand(task, message_name);
        }
    } catch (...) {
        (void)runtime.CleanupCommand(command);
        throw;
    }
    return runtime.CleanupCommand(command);
}

std::uint8_t ThrowPropertyActionTask::Update(
    const std::uint32_t task, const float delta,
    IThrowPropertyActionRuntime& runtime,
    IThrowPropertyTaskRuntime& throw_runtime,
    IUsePropertyTaskRuntime& property_runtime) {
    if (runtime.IsActionPending(task))
        runtime.ActorSetState(runtime.ReadWord(task + 60u), 1u);
    if (runtime.ReadByte(task + 409u) != 0u &&
        runtime.ReadByte(task + 408u) == 0u) {
        (void)ThrowPropertyTask::UpdatePosition(
            task + 240u, delta, throw_runtime);
        runtime.AccumulateElapsed(delta);
        if (runtime.ThrowComplete(task + 240u)) {
            UsePropertyTask::ProcessInternal(
                task + 328u, property_runtime);
            runtime.WriteByte(task + 408u, 1u);
        }
    }
    return 1u;
}

bool ThrowPropertyActionTask::CheckStatus(
    const std::uint32_t task, IThrowPropertyActionRuntime& runtime,
    IUsePropertyTaskRuntime& property_runtime) {
    return runtime.ReadByte(task + 408u) != 0u &&
           UsePropertyTask::CheckStatus(task + 328u, property_runtime) &&
           runtime.ReadByte(task + 410u) != 0u;
}

std::uint8_t ThrowPropertyActionTask::Start(
    const std::uint32_t task, IThrowPropertyActionRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    (void)ThrowPropertyActionHandlers::Subscribe(task, events);
    runtime.WriteByte(task + 408u, 0u);
    runtime.WriteByte(task + 409u, 0u);
    runtime.StartAction(task);
    return 1u;
}

std::uint32_t ThrowPropertyActionTask::HandleEffectEvent(
    const std::uint32_t task, const std::uint32_t event,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleEffectEvent(
        task, event, kEffectLayout, runtime);
}

std::uint32_t ThrowPropertyActionTask::HandleCompletionEvent(
    const std::uint32_t task, const std::uint32_t event,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleCompletionEvent(
        task, event, kEffectLayout, runtime);
}

int ThrowPropertyActionTask::HandleAnimationEvent(
    const std::uint32_t task, const PropertyTaskEffectHandle effect,
    const std::string_view event_name,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleAnimationEvent(
        task, effect, event_name, kEffectLayout, runtime);
}

}  // namespace pal4
