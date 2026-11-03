#pragma once
#include "pal4/native_combat_ending_tasks.h"
namespace pal4 {
class NativeCombatConsoleVisuals;class NativeCombatConsole;class INativeSkillSystemServices;
class NativeCombatResultActions;class NativeCombatTaskQueue;
class INativeCombatActorTurnServices;
namespace test {
void VerifyNativeCombatEndingSequence(NativeCombatEndingWindow&,NativeCombatPlayerManager&,
    NativeMilesSound2dRuntime&,NativeCombatSystemImage&,const NativeUpgradeRecordCatalog&,
    NativeCombatEndingTasks::ActorSkills,NativeCombatConsoleVisuals&,NativeCombatConsole&,INativeSkillSystemServices&);
void VerifyNativeCombatEndingSequenceActions(NativeCombatEndingWindow&,NativeCombatPlayerManager&,
    NativeMilesSound2dRuntime&,NativeCombatSystemImage&,const NativeUpgradeRecordCatalog&,
    NativeCombatEndingTasks::ActorSkills,NativeCombatTaskQueue&,
    const std::function<NativeCombatResultActions&(std::uint32_t)>&,INativeSkillSystemServices&,
    const std::function<INativeCombatActorTurnServices&(std::uint32_t)>&,
    const std::function<void(float)>&,const std::function<void()>&);
}
}
