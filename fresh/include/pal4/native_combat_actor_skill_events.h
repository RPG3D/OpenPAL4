#pragma once
#include <cstdint>
#include <memory>
#include <span>
#include "pal4/native_combat_skill_effect_services.h"

namespace pal4 {
class NativeCombatActorOwner;
class NativeCombatPlayerActorOwner;
class NativeScriptEventRouter;
class NativeSkillRecordCatalog;

// Shared Player/Monster notification slots280..328 and336 immunity. Borrows all three owners. Dispatch is
// synchronous; payloads must not be retained by event subscribers. Populated
// dynamic effects and linked damage properties require their real services.
class NativeCombatActorSkillEvents final {
public:
    NativeCombatActorSkillEvents(NativeCombatActorOwner&, NativeScriptEventRouter&,
                                NativeSkillRecordCatalog&);
    NativeCombatActorSkillEvents(NativeCombatPlayerActorOwner&,NativeScriptEventRouter&,NativeSkillRecordCatalog&);
    ~NativeCombatActorSkillEvents();
    void BindEffectServices(NativeCombatSkillEffectServices);
    // Combat Player845010 and Monster844D08 slot384/52E2E0, Skill EF1/BIND1.
    // Plain Player842F60 and snapshot8430E8 belong to separate base runtimes.
    ActorEffectHandle BuildTargetEffect(std::uint32_t skill);
    NativeCombatActorSkillEvents(const NativeCombatActorSkillEvents&) = delete;
    NativeCombatActorSkillEvents& operator=(const NativeCombatActorSkillEvents&) = delete;
    // context preserves original EBP for effect cleanup; it is separate from
    // the original stack arguments, including slot300's extra event word.
    // Notification return registers are not part of this public contract.
    void InvokeVirtual(std::uint32_t receiver, std::uint32_t slot,
                       std::span<const std::uint32_t> arguments,
                       std::uint32_t context = 0u);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
