#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
namespace pal4 {
class NativeCombatSkillActorRegistry;
class NativeCombatNormalAttackEffects;
class NativeCombatFormulaCatalog;
class NativeCombatEndSoundEvents;
class NativeCombatMonsterManager;
class NativeCombatPlayerManager;
// Original557960/557AC0 over actual actor skills, rosters, audio and effects.
// All dependencies are borrowed and must outlive the processor.
class NativeCombatNormalAttackProcessor final {
public:
    NativeCombatNormalAttackProcessor(NativeCombatSkillActorRegistry&,NativeCombatNormalAttackEffects&,
        const NativeCombatFormulaCatalog&,NativeCombatEndSoundEvents&,NativeCombatMonsterManager&,
        NativeCombatPlayerManager* players=nullptr);
    ~NativeCombatNormalAttackProcessor();
    NativeCombatNormalAttackProcessor(const NativeCombatNormalAttackProcessor&)=delete;
    NativeCombatNormalAttackProcessor& operator=(const NativeCombatNormalAttackProcessor&)=delete;
    std::uint8_t Process(std::uint32_t task);
    std::size_t ActiveIterators() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
