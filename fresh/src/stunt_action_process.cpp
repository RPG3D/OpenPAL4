#include "pal4/combat_effect_config_keys.h"
#include "pal4/stunt_action_task.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace pal4 {

void StuntActionTask::ProcessInternal(
    const std::uint32_t task, IStuntActionProcessRuntime& runtime,
    IStuntTaskRuntime& stunt_runtime) {
    const auto stunt = runtime.ReadWord(task + 324u);
    runtime.WriteByte(task + 332u, 0u);
    const auto target = runtime.ReadWord(task + 320u);
    struct EffectGuard final {
        IStuntActionProcessRuntime& runtime;
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
        runtime.PrepareImmediateStunt();
        runtime.WriteByte(task + 332u, 1u);
        if (group) {
            runtime.WriteFloat(task + 328u, 1.0f);
            (void)StuntTask::HandleActorInternal(
                task + 256u, candidate, stunt_runtime);
        } else {
            (void)StuntTask::HandleActor(task + 256u, stunt_runtime);
        }
    };

    std::string name(runtime.StuntName(stunt + 196u));
    std::transform(name.begin(), name.end(), name.begin(),
                   [](const unsigned char value) {
                       return static_cast<char>(std::tolower(value));
                   });
    if (name == "hook_area") {
        EffectGuard guard{runtime, runtime.ExecuteEffect(
            stunt + 176u, target, stunt + 192u)};
        if (runtime.NeedsUpdate(guard.effect)) bind(guard.effect, target);
        else immediate(target, false);
        return;
    }

    const auto kind = runtime.StuntTargetKind(stunt);
    if (kind != 3u && kind != 4u) {
        EffectGuard guard{runtime, runtime.EmptyEffect()};
        if (runtime.ActorReady(target) ||
            runtime.ReadWord(stunt + 4u) == 5u) {
            const auto created = runtime.ExecuteEffect(
                stunt + 176u, target, stunt + 192u);
            runtime.ReleaseEffect(guard.effect);
            guard.effect = created;
        }
        if (runtime.NeedsUpdate(guard.effect)) bind(guard.effect, target);
        else immediate(target, false);
        return;
    }

    const auto iterator = runtime.TargetIterator(target);
    struct IteratorGuard final {
        IStuntActionProcessRuntime& runtime;
        std::uint32_t iterator;
        ~IteratorGuard() {
            if (iterator) (void)runtime.ReleaseIterator(iterator);
        }
    } iterator_guard{runtime, iterator};
    if (!iterator) return;
    if (runtime.IteratorEmpty(iterator)) {
        runtime.PrepareImmediateStunt();
        runtime.WriteByte(task + 332u, 1u);
    }
    bool any_ready = false;
    while (!runtime.IteratorAtEnd(iterator)) {
        const auto candidate = runtime.IteratorCurrent(iterator);
        if (runtime.ActorReady(candidate) ||
            runtime.ReadWord(stunt + 4u) == 5u) {
            any_ready = true;
            EffectGuard guard{runtime, runtime.ExecuteEffect(
                stunt + 176u, candidate, stunt + 192u)};
            if (runtime.NeedsUpdate(guard.effect))
                bind(guard.effect, candidate);
            else immediate(candidate, true);
        }
        runtime.IteratorNext(iterator);
    }
    if (!any_ready) {
        runtime.PrepareImmediateStunt();
        runtime.WriteByte(task + 332u, 1u);
    }
}

}  // namespace pal4
