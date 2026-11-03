#pragma once
#include <cstdint>
namespace pal4 {
class NativeCombatActionController;class NativeCombatActionWindow;class NativeEventHandlerArray;class NativeCombatActorSkillRuntime;
class NativeInventorySession;class NativeCombatTargetSelection;class NativeCombatRosterTargets;class NativeScriptEventRouter;
namespace test {
void VerifyNativeCombatPropertyPreferred(NativeCombatActionWindow&,NativeEventHandlerArray&,
    NativeCombatActorSkillRuntime&,NativeInventorySession&,std::uint32_t property);
void StartNativeCombatPropertyFromSelector(NativeCombatActionController&,NativeCombatActionWindow&,NativeEventHandlerArray&,
    NativeCombatActorSkillRuntime&,NativeInventorySession&,NativeCombatTargetSelection&,NativeCombatRosterTargets&,
    NativeScriptEventRouter&,std::uint32_t target,std::uint32_t property);
}
}
