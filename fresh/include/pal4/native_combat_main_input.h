#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatMainWindow;
class NativeCombatMainJointSelection;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatTargetSelection;
class NativeCombatRosterTargets;
class NativeScriptEventRouter;
//54DA20 shared input algorithm over actual rosters, selection and router.
// Key arguments are original game event records, never injected OS input.
class NativeCombatMainInput final {
public:
    NativeCombatMainInput(NativeCombatMainWindow&,NativeCombatMainJointSelection&,
        NativeCombatPlayerManager&,NativeCombatMonsterManager&,NativeCombatTargetSelection&,
        NativeCombatRosterTargets&,NativeScriptEventRouter&);
    ~NativeCombatMainInput();
    bool Handle(std::uint32_t event);
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
