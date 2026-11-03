#pragma once
#include "pal4/native_combat_death_sequence_services.h"
namespace pal4 {
class NativeCombatActorSkillRuntime;
class NativePlayerCombatEvents;
class NativePlayerCollectionCatalog;
class INativeCombatActorTurnServices;
// Borrowed single-actor binding for the shared Dead/QuickDead child callbacks.
// All owners and the optional modeled death/geometry service outlive the tasks.
class NativeCombatActorDeathServices final : public INativeCombatDeathSequenceServices {
public:
    NativeCombatActorDeathServices(NativeCombatActorSkillRuntime&,NativePlayerCombatEvents&,
        const NativePlayerCollectionCatalog&,INativeCombatActorTurnServices* = nullptr);
    std::uint32_t ReadWord(std::uint32_t) override;
    std::uint8_t ReadByte(std::uint32_t) override;
    void WriteWord(std::uint32_t,std::uint32_t) override;
    void SetActorState(std::uint32_t,std::uint32_t,std::uint32_t) override;
    std::uint32_t EventDataByIndex(std::uint32_t) override;
    void ApplyFinalHitPropertyAction(std::uint32_t) override;
    void FireEvent(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void CallActor(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void SetProtectTarget(std::uint32_t,std::uint32_t,std::uint32_t) override;
    void ConstructBase(std::uint32_t) override;
    void SetStateOne(std::uint32_t) override;
private:
    void CheckActor(std::uint32_t) const;
    NativeCombatActorSkillRuntime& skills_;
    NativePlayerCombatEvents& events_;
    const NativePlayerCollectionCatalog& passives_;
    INativeCombatActorTurnServices* battle_;
};
}
