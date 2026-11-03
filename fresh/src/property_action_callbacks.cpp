#include "pal4/property_action_task.h"

#include "pal4/combat_effect_action_callbacks.h"

namespace pal4 {
namespace {

constexpr CombatEffectActionLayout kLayout{304u, 320u, 232u};

}  // namespace

PropertyTaskEffectHandle PropertyActionTask::CopyEventEffect(
    const std::uint32_t event, IPropertyActionEventRuntime& runtime) {
    return runtime.CopyEffectPair(event + 4u);
}

std::uint32_t PropertyActionTask::HandleEffectEvent(
    const std::uint32_t task, const std::uint32_t event,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleEffectEvent(
        task, event, kLayout, runtime);
}

std::uint32_t PropertyActionTask::HandleCompletionEvent(
    const std::uint32_t task, const std::uint32_t event,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleCompletionEvent(
        task, event, kLayout, runtime);
}

int PropertyActionTask::HandleAnimationEvent(
    const std::uint32_t task, const PropertyTaskEffectHandle effect,
    const std::string_view event_name,
    IPropertyActionEventRuntime& runtime) {
    return CombatEffectActionCallbacks::HandleAnimationEvent(
        task, effect, event_name, kLayout, runtime);
}

}  // namespace pal4
