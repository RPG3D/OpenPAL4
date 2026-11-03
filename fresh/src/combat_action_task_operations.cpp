#include "pal4/combat_action_task_operations.h"

namespace pal4 {

std::uint8_t CombatActionTaskOperations::SetTarget(
    const std::uint32_t task, ICombatActionTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (actor)
        runtime.SetActorTarget(actor, task + 64u,
                               runtime.ReadByte(task + 80u));
    return 1u;
}

std::uint8_t CombatActionTaskOperations::ProcessAction(
    const std::uint32_t task, ICombatActionTaskRuntime& runtime) {
    const auto actor = runtime.ReadWord(task + 60u);
    if (actor) {
        runtime.PrepareActionServices();
        runtime.StartActorAction(actor, task + 64u, 0u);
    }
    return 1u;
}

bool CombatActionTaskOperations::IsAnimationComplete(
    const std::uint32_t task, ICombatActionTaskRuntime& runtime) {
    const auto name_address = runtime.ReadWord(task + 68u);
    const auto name = name_address
        ? runtime.ReadCString(name_address) : std::string();
    const auto actor = runtime.ReadWord(task + 60u);
    if (runtime.CompareInsensitive(name,
                                   runtime.GetAnimationName(actor)) != 0)
        return true;
    return runtime.GetAnimationFlag(actor) > 0;
}

std::uint32_t CombatProcessMessageSubscriptions::Register(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    Clear(task, runtime);
    const auto animation_key = runtime.ReadWord(kAnimationKey);
    const auto first = runtime.Register(runtime.EventRouter(), 2u,
        animation_key, task, kAnimationCallback);
    runtime.WriteWord(task + 80u, first);
    runtime.WriteWord(task + 84u, animation_key);
    const auto effect_key = runtime.ReadWord(kEffectKey);
    const auto second = runtime.Register(runtime.EventRouter(), 2u,
        effect_key, task, kEffectCallback);
    runtime.WriteWord(task + 88u, second);
    runtime.WriteWord(task + 92u, effect_key);
    return second;
}

void CombatProcessMessageSubscriptions::Clear(
    const std::uint32_t task, IEventSubscriptionRuntime& runtime) {
    EventSubscriptionSet::Clear(task, 80u, 2u, runtime);
}

}  // namespace pal4
