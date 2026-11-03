#pragma once
#include "pal4/combat_script_task_operations.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatSkillActorRegistry;
class NativeCombatDamageCalculator;
class NativeCombatTaskQueue;
class NativeCombatReactionDomain;
// Borrows the actual battle damage, registry, queue and per-target reactions.
// Wounded defaults to the target's actual reaction domain. An explicit service
// can supply a different real Wounded owner when composing another battle.
class NativeCombatNormalAttackEffects final {
public:
    using ReactionResolver=std::function<NativeCombatReactionDomain&(std::uint32_t target)>;
    using RegisterWounded=std::function<std::uint8_t(std::uint32_t target,std::uint32_t attacker)>;
    NativeCombatNormalAttackEffects(NativeCombatSkillActorRegistry&,NativeCombatDamageCalculator&,
        NativeCombatTaskQueue&,ReactionResolver,RegisterWounded={});
    ~NativeCombatNormalAttackEffects();
    NativeCombatNormalAttackEffects(const NativeCombatNormalAttackEffects&)=delete;
    NativeCombatNormalAttackEffects& operator=(const NativeCombatNormalAttackEffects&)=delete;
    // Borrow the live76-byte NormalAttack image for this synchronous call.
    std::uint8_t Apply(std::uint32_t task,std::uint32_t target,NormalAttackActorBranch);
    // Original557AC0 property/probability dispatch, using the actual RNG.
    std::uint8_t Handle(std::uint32_t task,std::uint32_t target);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
