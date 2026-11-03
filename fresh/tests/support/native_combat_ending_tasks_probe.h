#pragma once
#include "pal4/native_combat_ending_tasks.h"
namespace pal4 { class NativeCombatConsoleVisuals;class NativeCombatConsole; }
namespace pal4::test {
void VerifyNativeCombatEndingTasks(NativeCombatEndingWindow&,NativeCombatPlayerManager&,
    NativeMilesSound2dRuntime&,NativeCombatSystemImage&,const NativeUpgradeRecordCatalog&,NativeCombatEndingTasks::ActorSkills,NativeCombatConsoleVisuals&,NativeCombatConsole&);
}
