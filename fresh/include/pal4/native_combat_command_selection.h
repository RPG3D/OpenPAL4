#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatPlayerManager;class NativeCombatMonsterManager;
class NativeCombatSkillActorRegistry;class NativeCombatRosterTargets;
class NativeCombatTargetCommandServices;class NativeCombatEventHandlers;
class NativeCombatSystemAccess;struct NativeCombatSystemImage;class NativeScriptEventRouter;
// Shared57C440 command routing,57C940/9A0/A00/CBD0 target preparation
// and57C920 state5. Execution remains owned by existing command domains.
class NativeCombatCommandSelection final {
public:
    NativeCombatCommandSelection(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        NativeCombatSkillActorRegistry&,NativeCombatRosterTargets&,NativeCombatTargetCommandServices&,
        NativeCombatEventHandlers&,NativeCombatSystemAccess&,NativeCombatSystemImage&,NativeScriptEventRouter&);
    ~NativeCombatCommandSelection();
    void Handle(std::uint32_t owner,std::uint32_t callback,std::uint32_t payload);
    void Close();
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
