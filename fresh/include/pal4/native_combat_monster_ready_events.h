#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatAiActions;
class NativeCombatSkillActorRegistry;
class NativeCombatRosterTargets;
class NativeCombatEventHandlers;
class NativeCombatSystemAccess;
struct NativeCombatSystemImage;
class NativeScriptEventRouter;
// Actual57C370 consumer, borrowed by the extended handler subscription owner.
// VIP outcome dispatch is preserved; complete combat_over consumers remain
// separate. AI action domains must be available for the selected branch.
class NativeCombatMonsterReadyEvents final {
public:
    NativeCombatMonsterReadyEvents(NativeCombatAiActions&,NativeCombatSkillActorRegistry&,
        NativeCombatRosterTargets&,NativeCombatEventHandlers&,NativeCombatSystemAccess&,
        NativeCombatSystemImage&,NativeScriptEventRouter&);
    ~NativeCombatMonsterReadyEvents();
    void Handle(std::uint32_t owner,std::uint32_t payload);
    void Close();
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
