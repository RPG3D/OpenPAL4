#pragma once
#include <cstdint>
#include <span>
#include <functional>
namespace pal4 {
class NativeCombatActionController;class NativeCombatActionWindow;class NativeCombatActorSkillRuntime;
class NativeEventHandlerArray;class NativeMagicWindowRecords;
class NativeMagicWindowLayout;
class NativeCombatTargetSelection;class NativeCombatMonsterManager;
class NativeInventorySession;class NativeSkillRecordCatalog;
namespace test {
void ReportNativeCombatPlayerInitialStats(std::span<NativeCombatActorSkillRuntime* const>);
void TeachNextPartyMagicForNaturalProbe(NativeMagicWindowLayout&,const std::function<void(unsigned)>& key,
    const std::function<void()>& rethrow);
// Test input only. Borrows live stock/records and remembers the selected item
// target across the original popup/target phases. Never supplies inventory,
// writes HP/MP/phase/result or changes RNG. Production owns every game action.
class NativeCombatPlayerInputPolicy final {
public:
    NativeCombatPlayerInputPolicy(NativeInventorySession&,NativeSkillRecordCatalog&,NativeCombatMonsterManager&);
    std::uint32_t NextPlayerKey(NativeCombatActionController&,NativeCombatActionWindow&,
        NativeEventHandlerArray&,std::span<NativeCombatActorSkillRuntime* const>,const NativeMagicWindowRecords&,bool scripted);
    std::uint32_t NextTargetKey(NativeCombatTargetSelection&);
private:
    NativeInventorySession& inventory_;NativeSkillRecordCatalog& skills_;NativeCombatMonsterManager& monsters_;
    std::uint32_t property_{},target_{};
    bool PlanRecovery(std::span<NativeCombatActorSkillRuntime* const>,NativeCombatActorSkillRuntime&,bool needs_mp);
};
// Controlled existing Property fixture: drive the same read-only policy through
// actual popup and target key handlers. Stock/HP operands belong to its caller.
void StartNativeCombatRecoveryPolicyProbe(NativeCombatActionController&,NativeCombatActionWindow&,NativeEventHandlerArray&,
    std::span<NativeCombatActorSkillRuntime* const>,const NativeMagicWindowRecords&,NativeInventorySession&,
    NativeSkillRecordCatalog&,NativeCombatMonsterManager&,NativeCombatTargetSelection&);
}
}
