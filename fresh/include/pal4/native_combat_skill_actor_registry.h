#pragma once
#include <cstddef>
#include <cstdint>
#include <map>

namespace pal4 {
class NativeCombatActorSkillRuntime;
class NativeScriptEventRouter;
class NativeSkillRecordCatalog;
class INativeSkillSystemServices;

// Host routing domain only; this is not original Combat actor registration.
// Borrows all services and actors. The registry must outlive its runtimes.
class NativeCombatSkillActorRegistry final {
public:
    NativeCombatSkillActorRegistry(NativeScriptEventRouter&,NativeSkillRecordCatalog&,
                                  INativeSkillSystemServices&);
    NativeCombatSkillActorRegistry(const NativeCombatSkillActorRegistry&)=delete;
    NativeCombatSkillActorRegistry& operator=(const NativeCombatSkillActorRegistry&)=delete;
    [[nodiscard]] std::size_t Count() const noexcept { return actors_.size(); }
    [[nodiscard]] bool Contains(std::uint32_t address) const noexcept { return actors_.contains(address); }
private:
    friend class NativeCombatActorSkillRuntime;
    friend class NativeCombatMonsterManager;
    friend class NativeCombatDamageCalculator;
    friend class NativeCombatMissActions;
    friend class NativeCombatFendOffActions;
    friend class NativeCombatNormalAttackEffects;
    friend class NativeCombatNormalAttackProcessor;
    friend class NativeCombatWoundedActions;
    friend class NativeCombatPhysicalAttackActions;
    friend class NativeCombatActorReactionServices;
    friend class NativeCombatAiActions;
    friend class NativeCombatMonsterReadyEvents;
    friend class NativeCombatPlayerReadyEvents;
    friend class NativeCombatActionFrame;
    friend class NativeCombatRoleStateFrame;
    friend class NativeCombatMainUiEvents;
    friend class NativeCombatCommandSelection;
    friend class NativeCombatActionController;
    friend class NativeCombatFleeActions;
    friend class NativeCombatResultActions;
    friend class NativeCombatSystemUpdate;
    friend class NativeCombatActorExitEvents;
    friend class NativeCombatSkillActionRuntime;
    friend class NativeCombatSkillDomain;
    friend class NativeCombatPropertyRuntime;
    friend class NativeCombatPropertyActionRuntime;
    friend class NativeCombatThrowProperty;
    friend class NativeCombatPropertyDomain;
    struct Entry { NativeCombatActorSkillRuntime* runtime; std::uint64_t generation; };
    NativeScriptEventRouter& router_;
    NativeSkillRecordCatalog& catalog_;
    INativeSkillSystemServices& system_;
    std::map<std::uint32_t,Entry> actors_;
    std::uint64_t next_generation_{1u};
    unsigned depth_{};
    void RequireIdle() const;
    void Register(std::uint32_t,NativeCombatActorSkillRuntime&);
    void Erase(std::uint32_t,NativeCombatActorSkillRuntime&) noexcept;
    NativeCombatActorSkillRuntime& Resolve(std::uint32_t) const;
    std::uint64_t Token(std::uint32_t) const;
};
}
