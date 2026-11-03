#pragma once
#include <functional>
namespace pal4 {
class NativeCombatEndingLevelUp;class NativeCombatEndingWindow;class NativeCombatActorSkillRuntime;
class NativeCombatTaskQueue;class NativeCombatHints;class NativeMilesSound2dRuntime;class NativeGameTimerQueue;
class NativeGameDatabase;class NativeEffectSceneSession;
namespace test {
void VerifyNativeCombatEndingLevelUp(NativeCombatEndingLevelUp&,NativeCombatEndingWindow&,NativeCombatActorSkillRuntime&,
    NativeCombatTaskQueue&,NativeCombatHints&,NativeMilesSound2dRuntime&,NativeGameTimerQueue&,
    const NativeGameDatabase&,NativeEffectSceneSession&,const std::function<void(float)>&,const std::function<void()>&);
}
}
