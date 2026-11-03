#include "pal4/cast_property_action_task.h"

#include "pal4/combat_effect_action_callbacks.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {
namespace {

constexpr CombatEffectActionLayout kEffectLayout{296u, 326u, 312u};

}  // namespace

std::uint32_t CastPropertyActionHandlers::Subscribe(
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

void CastPropertyActionHandlers::Clear(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    ThrowPropertyActionHandlers::Clear(task, runtime);
}

void CastPropertyActionTask::Destroy(
    const std::uint32_t task, ICastPropertyActionRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    runtime.WriteWord(task, kVtable);
    CastPropertyActionHandlers::Clear(task, events);
    runtime.ResetEffect(task + 312u);
    runtime.ResetEffect(task + 304u);
    runtime.DestroyEmbedded(task + 232u);
    runtime.DestroyBase(task);
}

int CastPropertyActionTask::ProcessCommand(
    const std::uint32_t task, const std::string_view message_name,
    ICastPropertyActionRuntime& runtime) {
    std::string command(message_name);
    std::transform(command.begin(), command.end(), command.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    try {
        if (command.size() >= 3u && command.compare(0u, 3u, "atk") == 0) {
            runtime.WriteByte(task + 325u, 1u);
            auto effect = runtime.CreateActorEffect(
                runtime.ReadWord(task + 296u), "EFF15", kEffectHook);
            runtime.ResetTemporaryEffect(effect);
        } else {
            runtime.ProcessAnimationCommand(task, message_name);
        }
    } catch (...) {
        (void)runtime.CleanupCommand(command);
        throw;
    }
    return runtime.CleanupCommand(command);
}

std::uint8_t CastPropertyActionTask::Update(
    const std::uint32_t task, const float delta,
    ICastPropertyActionRuntime& runtime,
    IUsePropertyTaskRuntime& property_runtime) {
    if (runtime.IsActionPending(task))
        runtime.ActorSetState(runtime.ReadWord(task + 60u), 1u);
    if (runtime.ReadByte(task + 325u) != 0u &&
        runtime.ReadByte(task + 324u) == 0u) {
        const auto elapsed = runtime.ReadFloat(task + 320u) + delta;
        runtime.WriteFloat(task + 320u, elapsed);
        const auto duration = runtime.ReadFloat(
            runtime.ReadWord(task + 300u) + 164u);
        if (elapsed > duration) {
            UsePropertyTask::ProcessInternal(task + 232u,
                                             property_runtime);
            runtime.WriteByte(task + 324u, 1u);
        }
    }
    return 1u;
}

bool CastPropertyActionTask::CheckStatus(
    const std::uint32_t task, ICastPropertyActionRuntime& runtime,
    IUsePropertyTaskRuntime& property_runtime) {
    return runtime.ReadByte(task + 324u) != 0u &&
           UsePropertyTask::CheckStatus(task + 232u,
                                        property_runtime) &&
           runtime.ReadByte(task + 326u) != 0u;
}

std::uint8_t CastPropertyActionTask::Start(
    const std::uint32_t task, ICastPropertyActionRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    (void)CastPropertyActionHandlers::Subscribe(task, events);
    runtime.WriteByte(task + 324u, 0u);
    runtime.WriteByte(task + 325u, 0u);
    runtime.WriteWord(task + 320u, 0u);
    runtime.StartAction(task);
    return 1u;
}

std::uint8_t CastPropertyActionTask::Stop(
    const std::uint32_t task, const std::uint32_t frame_context,
    ICastPropertyActionRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    CastPropertyActionHandlers::Clear(task, events);
    runtime.StopAction(task, frame_context);
    runtime.ResetEmbedded(task + 232u);
    return 1u;
}

std::uint32_t CastPropertyActionTask::HandleEffectEvent(
    const std::uint32_t task, const std::uint32_t event,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleEffectEvent(
        task, event, kEffectLayout, runtime);
}

std::uint32_t CastPropertyActionTask::HandleCompletionEvent(
    const std::uint32_t task, const std::uint32_t event,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleCompletionEvent(
        task, event, kEffectLayout, runtime);
}

int CastPropertyActionTask::HandleAnimationEvent(
    const std::uint32_t task, const PropertyTaskEffectHandle effect,
    const std::string_view event_name,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleAnimationEvent(
        task, effect, event_name, kEffectLayout, runtime);
}

}  // namespace pal4
