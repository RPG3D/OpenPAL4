#include "pal4/combat_effect_action_callbacks.h"

#include <algorithm>
#include <cctype>

namespace pal4 {
namespace {

bool EqualsIgnoreCase(const std::string_view actual,
                      const std::string_view expected) {
    return actual.size() == expected.size() &&
           std::equal(actual.begin(), actual.end(), expected.begin(),
                      [](const unsigned char left,
                         const unsigned char right) {
                          return std::tolower(left) == std::tolower(right);
                      });
}

}  // namespace

std::uint32_t CombatEffectActionCallbacks::HandleEffectEvent(
    const std::uint32_t task, const std::uint32_t event,
    const CombatEffectActionLayout layout,
    IPropertyActionEventRuntime& runtime) {
    auto effect = PropertyActionTask::CopyEventEffect(event, runtime);
    bool ignored = false;
    int result = 0;
    try {
        ignored = EqualsIgnoreCase(runtime.EffectName(effect), "EFF13");
    } catch (...) {
        (void)runtime.ResetEffectPair(effect);
        throw;
    }
    result = runtime.ResetEffectPair(effect);
    if (ignored) return static_cast<std::uint32_t>(result);

    effect = PropertyActionTask::CopyEventEffect(event, runtime);
    try {
        ignored = EqualsIgnoreCase(runtime.EffectName(effect), "EFF14");
    } catch (...) {
        (void)runtime.ResetEffectPair(effect);
        throw;
    }
    result = runtime.ResetEffectPair(effect);
    if (ignored) return static_cast<std::uint32_t>(result);

    const auto owner = runtime.EffectOwner(event);
    if (owner != runtime.ReadWord(task + layout.owner_offset)) return owner;
    effect = PropertyActionTask::CopyEventEffect(event, runtime);
    try {
        runtime.WriteWord(task + layout.effect_offset, effect.first);
        runtime.WriteWord(task + layout.effect_offset + 4u,
                          effect.second);
    } catch (...) {
        (void)runtime.ResetEffectPair(effect);
        throw;
    }
    result = runtime.ResetEffectPair(effect);
    runtime.WriteByte(task + layout.completion_offset, 0u);
    return static_cast<std::uint32_t>(result);
}

std::uint32_t CombatEffectActionCallbacks::HandleCompletionEvent(
    const std::uint32_t task, const std::uint32_t event,
    const CombatEffectActionLayout layout,
    IPropertyActionEventRuntime& runtime) {
    const auto first = runtime.ReadWord(event);
    if (first != runtime.ReadWord(task + layout.effect_offset)) return first;
    const auto second = runtime.ReadWord(
        task + layout.effect_offset + 4u);
    if (runtime.ReadWord(event + 4u) != second) return second;
    const auto result = runtime.PrepareEffectCompletion({first, second});
    runtime.WriteByte(task + layout.completion_offset, 1u);
    return result;
}

int CombatEffectActionCallbacks::HandleAnimationEvent(
    const std::uint32_t task, PropertyTaskEffectHandle effect,
    const std::string_view event_name,
    const CombatEffectActionLayout layout,
    IPropertyActionEventRuntime& runtime) {
    try {
        if (EqualsIgnoreCase(event_name, "endact") &&
            effect.first == runtime.ReadWord(task + layout.effect_offset) &&
            effect.second == runtime.ReadWord(
                task + layout.effect_offset + 4u)) {
            (void)runtime.PrepareEffectCompletion(effect);
            runtime.WriteByte(task + layout.completion_offset, 1u);
        }
    } catch (...) {
        (void)runtime.ResetEffectPair(effect);
        throw;
    }
    return runtime.ResetEffectPair(effect);
}

}  // namespace pal4
