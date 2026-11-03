#include "pal4/combat_effect_config_keys.h"
#include "pal4/stunt_action_task.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {

int StuntActionTask::HandleAnimationEvent(
    const std::uint32_t task, const std::uint32_t effect,
    const std::string_view event_name,
    IStuntActionProcessRuntime& runtime,
    IStuntTaskRuntime& stunt_runtime) {
    struct AnimationGuard final {
        IStuntActionProcessRuntime& runtime;
        std::uint32_t effect;
        bool active = true;
        ~AnimationGuard() {
            if (active) (void)runtime.ResetAnimationEffect(effect);
        }
    } guard{runtime, effect};
    std::string name(event_name);
    std::transform(name.begin(), name.end(), name.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    if (name == "atk") {
        const auto attached = runtime.ConfigValue(effect, CombatEffectConfigKeys::AttachedCast);
        const auto caster = runtime.ConfigValue(effect, CombatEffectConfigKeys::Cast);
        if (attached == task) {
            ProcessInternal(task, runtime, stunt_runtime);
        } else if (caster == task) {
            const auto stunt = runtime.ReadWord(task + 324u);
            std::string stunt_name(runtime.StuntName(stunt + 196u));
            std::transform(stunt_name.begin(), stunt_name.end(),
                           stunt_name.begin(),
                           [](const unsigned char value) {
                               return static_cast<char>(std::tolower(value));
                           });
            if (stunt_name == "hook_area") {
                (void)StuntTask::HandleActor(task + 256u, stunt_runtime);
            } else {
                const auto target = runtime.ConfigValue(effect, CombatEffectConfigKeys::Target);
                runtime.WriteFloat(task + 328u, 1.0f);
                (void)StuntTask::HandleActorInternal(
                    task + 256u, target, stunt_runtime);
            }
        }
    } else if (name == "endact") {
        (void)runtime.ConfigValue(effect, CombatEffectConfigKeys::AttachedCast);
        if (runtime.ConfigValue(effect, CombatEffectConfigKeys::Cast) == task) {
            runtime.PrepareImmediateStunt();
            runtime.WriteByte(task + 332u, 1u);
        }
    }
    guard.active = false;
    return runtime.ResetAnimationEffect(effect);
}

std::uint32_t StuntActionTask::HandleActorEvent(
    const std::uint32_t task, const std::uint32_t event,
    IStuntActionActorEventRuntime& runtime) {
    const auto first = runtime.ReadWord(event + 8u);
    const auto second = runtime.ReadWord(event + 12u);
    const auto difference = static_cast<std::int32_t>(second - first);
    const auto actor = runtime.EventActor(event);
    const auto source = runtime.ReadWord(task + 60u);
    if (actor == source || runtime.ReadWord(event + 4u) != 7u ||
        difference >= 0) return source;
    const auto actor_value = runtime.CallActor(actor, 52u, 7u);
    const auto result = actor_value - first;
    if (static_cast<std::int32_t>(result) < 0)
        return runtime.CallActor(actor, 340u, source);
    return result;
}

void StuntActionTask::HandleCompletionEvent(
    const std::uint32_t task, const std::uint32_t effect,
    IStuntActionProcessRuntime& runtime) {
    if (runtime.ConfigValue(effect, CombatEffectConfigKeys::Cast) != task) return;
    runtime.PrepareImmediateStunt();
    runtime.WriteByte(task + 332u, 1u);
}

std::uint32_t StuntActionTask::HandleCombatEvent(
    const std::uint32_t task, const std::uint32_t event,
    IStuntActionProcessRuntime& runtime,
    IStuntTaskRuntime& stunt_runtime) {
    const auto stunt = runtime.ReadWord(task + 324u);
    const auto enabled = stunt_runtime.StuntIncludesSelf(stunt);
    if (static_cast<std::uint8_t>(enabled) == 0u) return enabled;
    const auto actor = runtime.EventActor(event);
    const auto source = runtime.ReadWord(task + 60u);
    if (actor == source) return source;
    const auto state = runtime.ReadWord(event + 4u);
    if (state > 1u) return state;
    const auto result = runtime.CallActor(actor,
                                          state == 0u ? 136u : 144u, 0u);
    if (!result) return result;
    return runtime.CallActor(actor, 340u, source);
}

}  // namespace pal4
