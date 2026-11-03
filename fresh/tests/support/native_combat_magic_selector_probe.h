#pragma once
#include <cstdint>
namespace pal4 {
class NativeCombatActionController;class NativeCombatActionWindow;class NativeEventHandlerArray;
class NativeCombatActorSkillRuntime;class NativeMagicWindowRecords;class NativeCombatTargetSelection;
class NativeCombatRosterTargets;class NativeScriptEventRouter;
namespace test {
// Uses actual learned IDs only. Returns false before any UI mutation if absent.
bool StartNativeCombatMagicFromSelector(NativeCombatActionController&,NativeCombatActionWindow&,NativeEventHandlerArray&,
    NativeCombatActorSkillRuntime&,const NativeMagicWindowRecords&,NativeCombatTargetSelection&,
    NativeCombatRosterTargets&,NativeScriptEventRouter&,std::uint32_t target,std::uint32_t magic);
}
}
