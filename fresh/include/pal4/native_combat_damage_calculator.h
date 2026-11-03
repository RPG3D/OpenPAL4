#pragma once
#include "pal4/combat_normal_damage_calculator.h"
#include <memory>
#include <functional>
#include <vector>
namespace pal4 {
class NativeCombatSkillActorRegistry;
class NativeCombatFormulaCatalog;
// Borrows the actual shared skill routing domain, formula catalog and RNG.
// Calculate methods return signed deltas; ApplyAction executes original9116/9117
// through the same validated formula/actor runtime and actual HP event setters.
class NativeCombatDamageCalculator final {
public:
    NativeCombatDamageCalculator(NativeCombatSkillActorRegistry&,const NativeCombatFormulaCatalog&);
    ~NativeCombatDamageCalculator();
    NativeCombatDamageCalculator(const NativeCombatDamageCalculator&)=delete;
    NativeCombatDamageCalculator& operator=(const NativeCombatDamageCalculator&)=delete;
    [[nodiscard]] CombatDamagePair Calculate(std::uint32_t attacker,std::uint32_t defender,bool skip_half=false);
    // Original557D20's branch uses57D280, not the spell damage57D6D0 formula.
    [[nodiscard]] CombatDamagePair CalculateExtended(std::uint32_t attacker,std::uint32_t defender,bool skip_half=false);
    using RelatedTargets=std::function<std::vector<std::uint32_t>(std::uint32_t)>;
    // Original9116/9117 callbacks; applies shared skill/magic damage through actor84.
    void ApplyAction(std::uint32_t callback,std::uint32_t record,std::uint32_t source,
        std::uint32_t target,RelatedTargets);
    [[nodiscard]] bool CheckHit(std::uint32_t attacker,std::uint32_t defender);
    [[nodiscard]] bool CheckDodge(std::uint32_t attacker,std::uint32_t defender,float extra=0.f);
    // Original57DA30 over actual formula9 and the shared game RNG.
    [[nodiscard]] bool CheckCritical(float numerator,float denominator);
    [[nodiscard]] bool CheckFloatProperty(std::uint32_t attacker,std::uint32_t defender);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
