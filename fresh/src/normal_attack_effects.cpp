#include "pal4/normal_attack_effects.h"

namespace pal4 {
namespace {

std::uint8_t ApplyDamage(const std::uint32_t task,
                         const std::uint32_t target,
                         const NormalAttackDamageKind kind,
                         INormalAttackEffectRuntime& runtime) {
    const auto ready = runtime.ActorReady(target);
    if (!ready) return ready;
    const auto attacker = runtime.ReadWord(task + 60u);
    if (target == attacker)
        return static_cast<std::uint8_t>(attacker);

    const auto damage = runtime.CalculateDamage(
        attacker, target, runtime.ReadByte(task + 73u), kind);
    runtime.NotifyAttacker(attacker, target, kind);
    runtime.NotifyTarget(target, attacker);
    const auto before = runtime.Value7(target);
    if (runtime.HasProperty12(target)) {
        const auto primary = runtime.FloatToInt(damage.target);
        if (primary) {
            runtime.AddValue7(target, primary);
            if (runtime.Value7(target) > 0 &&
                !runtime.BlocksReflectedDamage(attacker)) {
                runtime.AddValue7(attacker,
                                  runtime.FloatToInt(damage.attacker));
            }
        }
    }

    runtime.TransferScaledMetric(attacker, target, runtime.ReadFloat(task + 68u));
    runtime.NotifyCombatState(attacker, 4u, target);
    runtime.NotifyCombatState(attacker, 13u, attacker);
    runtime.NotifyCombatState(target, 3u, target);
    runtime.NotifyCombatState(target, 12u, attacker);

    const auto after = runtime.Value7(target);
    if (before > after)
        return runtime.CombatWounded(target, attacker);
    const auto second_after = runtime.Value7(target);
    if (before == second_after)
        return runtime.UnchangedTarget(target, attacker);
    return static_cast<std::uint8_t>(second_after);
}

}  // namespace

std::uint8_t NormalAttackEffects::Attack(
    const std::uint32_t task, const std::uint32_t target,
    INormalAttackEffectRuntime& runtime) {
    return ApplyDamage(task, target, NormalAttackDamageKind::Physical, runtime);
}

std::uint8_t NormalAttackEffects::Magic(
    const std::uint32_t task, const std::uint32_t target,
    INormalAttackEffectRuntime& runtime) {
    return ApplyDamage(task, target, NormalAttackDamageKind::Magic, runtime);
}

std::uint8_t NormalAttackEffects::Item(
    const std::uint32_t task, const std::uint32_t target,
    INormalAttackEffectRuntime& runtime) {
    const auto ready = runtime.ActorReady(target);
    if (!ready) return ready;
    const auto attacker = runtime.ReadWord(task + 60u);
    if (target == attacker) return ready;
    const auto state = runtime.TargetState(target);
    if (state == 4u) return static_cast<std::uint8_t>(state);
    runtime.AddValue7(target, -1);
    return runtime.RegisterFendOff(attacker, target);
}

std::uint8_t NormalAttackEffects::Support(
    const std::uint32_t task, const std::uint32_t target,
    INormalAttackEffectRuntime& runtime) {
    const auto ready = runtime.ActorReady(target);
    if (!ready) return ready;
    const auto attacker = runtime.ReadWord(task + 60u);
    if (target == attacker) return ready;
    return runtime.RegisterMiss(attacker, target);
}

}  // namespace pal4
