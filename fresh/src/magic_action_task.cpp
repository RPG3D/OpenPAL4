#include "pal4/combat_effect_config_keys.h"
#include "pal4/magic_action_task.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {

std::uint32_t CombatActionHandlers::Subscribe(
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

void CombatActionHandlers::Clear(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(task, 216u, 5u, runtime);
}

std::uint8_t MagicActionTask::Process(
    const std::uint32_t task, IMagicActionTaskRuntime& runtime,
    IEventSubscriptionRuntime& events, ICastMagicTaskRuntime& cast) {
    runtime.StartAction(task);
    (void)CombatActionHandlers::Subscribe(task, events);
    (void)CastMagicTask::SetTarget(task + 256u, cast);
    return 1u;
}

std::uint8_t MagicActionTask::Stop(
    const std::uint32_t task, const std::uint32_t frame_context,
    IMagicActionTaskRuntime& runtime, IEventSubscriptionRuntime& events) {
    CombatActionHandlers::Clear(task, events);
    runtime.StopAction(task, frame_context);
    return 1u;
}

void MagicActionTask::Destroy(
    const std::uint32_t task, IMagicActionTaskRuntime& runtime,
    IEventSubscriptionRuntime& events) {
    runtime.WriteWord(task, kVtable);
    CombatActionHandlers::Clear(task, events);
    runtime.DestroyCastTask(task + 256u);
    runtime.DestroyBase(task);
}

int MagicActionTask::ProcessCommand(
    const std::uint32_t task, const std::string_view message_name,
    IMagicActionTaskRuntime& runtime) {
    std::string command(message_name);
    std::transform(command.begin(), command.end(), command.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    if (command.size() > 2u && command.compare(0u, 3u, "atk") == 0)
        runtime.ProcessAttack(task);
    else
        runtime.ProcessAnimationCommand(task, message_name);
    return runtime.CleanupCommand(command);
}

void MagicActionTask::ProcessInternal(
    const std::uint32_t task, IMagicActionProcessRuntime& runtime,
    ICastMagicTaskRuntime& cast) {
    const auto magic = runtime.ReadWord(task + 324u);
    runtime.WriteByte(task + 332u, 0u);
    const auto target = runtime.ReadWord(task + 320u);
    struct EffectGuard final {
        IMagicActionProcessRuntime& runtime;
        std::uint32_t effect;
        ~EffectGuard() { runtime.ReleaseEffect(effect); }
    };

    const auto bind = [&](const std::uint32_t effect,
                          const std::uint32_t candidate) {
        runtime.BindEffect(effect, CombatEffectConfigKeys::Cast, task);
        runtime.BindEffect(effect, CombatEffectConfigKeys::Target, candidate);
    };
    const auto immediate = [&](const std::uint32_t candidate,
                               const bool group) {
        runtime.PrepareImmediateCast();
        runtime.WriteByte(task + 332u, 1u);
        if (group) {
            runtime.WriteFloat(task + 328u, 1.0f);
            (void)CastMagicTask::HandleActorInternal(
                task + 256u, candidate, 1, cast);
        } else {
            (void)CastMagicTask::HandleActor(task + 256u, 1, cast);
        }
    };

    std::string name(runtime.MagicName(magic + 140u));
    std::transform(name.begin(), name.end(), name.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    if (name == "hook_area") {
        EffectGuard guard{runtime, runtime.ExecuteEffect(
            magic + 120u, target, magic + 136u)};
        if (runtime.NeedsUpdate(guard.effect)) bind(guard.effect, target);
        else immediate(target, false);
        return;
    }

    const auto mode = runtime.MagicMode(magic);
    if (mode != 3u && mode != 4u) {
        EffectGuard guard{runtime, runtime.EmptyEffect()};
        if (runtime.ActorReady(target) ||
            runtime.ReadWord(magic + 8u) == 5u) {
            const auto created = runtime.ExecuteEffect(
                magic + 120u, target, magic + 136u);
            runtime.ReleaseEffect(guard.effect);
            guard.effect = created;
        }
        if (runtime.NeedsUpdate(guard.effect)) bind(guard.effect, target);
        else immediate(target, false);
        return;
    }

    const auto iterator = runtime.TargetIterator(target);
    struct IteratorGuard final {
        IMagicActionProcessRuntime& runtime;
        std::uint32_t iterator;
        ~IteratorGuard() {
            if (iterator) (void)runtime.ReleaseIterator(iterator);
        }
    } iterator_guard{runtime, iterator};
    if (!iterator) return;
    if (runtime.IteratorEmpty(iterator)) {
        runtime.PrepareImmediateCast();
        runtime.WriteByte(task + 332u, 1u);
    }
    bool any_ready = false;
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (runtime.ActorReady(candidate) ||
            runtime.ReadWord(magic + 8u) == 5u) {
            any_ready = true;
            EffectGuard guard{runtime, runtime.ExecuteEffect(
                magic + 120u, candidate, magic + 136u)};
            if (runtime.NeedsUpdate(guard.effect))
                bind(guard.effect, candidate);
            else immediate(candidate, true);
        }
        runtime.IteratorNext(iterator);
    }
    if (!any_ready) {
        runtime.PrepareImmediateCast();
        runtime.WriteByte(task + 332u, 1u);
    }
}

int MagicActionTask::HandleAnimationEvent(
    const std::uint32_t task, const std::uint32_t effect,
    const std::string_view event_name, IMagicActionProcessRuntime& runtime,
    ICastMagicTaskRuntime& cast) {
    struct AnimationGuard final {
        IMagicActionProcessRuntime& runtime;
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
            ProcessInternal(task, runtime, cast);
        } else if (caster == task) {
            const auto magic = runtime.ReadWord(task + 324u);
            std::string magic_name(runtime.MagicName(magic + 140u));
            std::transform(magic_name.begin(), magic_name.end(),
                           magic_name.begin(),
                           [](const unsigned char value) {
                               return static_cast<char>(std::tolower(value));
                           });
            if (magic_name == "hook_area") {
                (void)CastMagicTask::HandleActor(task + 256u, 1, cast);
            } else {
                const auto target = runtime.ConfigValue(effect, CombatEffectConfigKeys::Target);
                runtime.WriteFloat(task + 328u, 1.0f);
                (void)CastMagicTask::HandleActorInternal(
                    task + 256u, target, 1, cast);
            }
        }
    } else if (name == "endact") {
        (void)runtime.ConfigValue(effect, CombatEffectConfigKeys::AttachedCast);
        if (runtime.ConfigValue(effect, CombatEffectConfigKeys::Cast) == task) {
            runtime.PrepareImmediateCast();
            runtime.WriteByte(task + 332u, 1u);
        }
    }
    guard.active = false;
    return runtime.ResetAnimationEffect(effect);
}

void MagicActionTask::HandleCompletionEvent(
    const std::uint32_t task, const std::uint32_t effect,
    IMagicActionProcessRuntime& runtime) {
    if (runtime.ConfigValue(effect, CombatEffectConfigKeys::Cast) != task) return;
    runtime.PrepareImmediateCast();
    runtime.WriteByte(task + 332u, 1u);
}

int MagicActionTask::HandleCombatEvent(
    const std::uint32_t task, const std::uint32_t event,
    IMagicActionProcessRuntime& runtime, ICastMagicTaskRuntime& cast) {
    const auto magic = runtime.ReadWord(task + 324u);
    const auto enabled = cast.MagicRejectsSelf(magic);
    if (static_cast<std::uint8_t>(enabled) == 0u) return enabled;
    const auto actor = runtime.EventActor(event);
    const auto caster = runtime.ReadWord(task + 60u);
    if (actor == caster) return static_cast<int>(caster);
    const auto state = runtime.ReadWord(event + 4u);
    if (state > 1u) return static_cast<int>(state);
    const auto result = cast.CallActor(actor, state == 0u ? 136u : 144u,
                                       0u, 0u);
    if (result == 0) return result;
    return cast.CallActor(actor, 340u, caster, 0u);
}

}  // namespace pal4
