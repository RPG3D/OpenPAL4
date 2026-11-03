#pragma once
#include <array>
#include <functional>
#include <string>
namespace pal4 {
class NativeCombatResultActions;class NativeCombatTaskQueue;class NativeCombatActorSkillRuntime;
class INativeSkillSystemServices;class NativeScriptEventRouter;class NativeCombatReactionDomain;
namespace test {
void VerifyNativeCombatWin(NativeCombatResultActions&,NativeCombatTaskQueue&,NativeCombatActorSkillRuntime&,
    NativeCombatReactionDomain&,INativeSkillSystemServices&,NativeScriptEventRouter&,
    const std::function<void(float)>& advance,const std::function<std::array<float,3>()>& formation,
    const std::function<std::string()>& animation,const std::function<void()>& restore_motion);
}
}
