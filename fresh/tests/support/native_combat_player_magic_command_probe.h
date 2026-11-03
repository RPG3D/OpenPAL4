#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>
namespace pal4 {
class NativeCombatCommandActions;class NativeCombatEventHandlers;class NativeCombatActorSkillRuntime;
class NativeCombatMonsterManager;class NativeCombatSkillDomain;class NativeMagicWindowRecords;
class NativeCombatTaskQueue;class NativeCombatPlayerActionMemory;class NativeScriptEventRouter;
namespace test {
void VerifyNativeCombatPlayerMagicCommand(NativeCombatCommandActions&,NativeCombatEventHandlers&,
    std::span<NativeCombatActorSkillRuntime* const>,NativeCombatMonsterManager&,const NativeMagicWindowRecords&,
    NativeCombatTaskQueue&,NativeCombatPlayerActionMemory&,NativeScriptEventRouter&,
    const std::function<NativeCombatSkillDomain&(std::uint32_t)>&,const std::function<void()>&,
    const std::function<bool(NativeCombatActorSkillRuntime&,std::uint32_t,std::uint32_t)>& start_ui,
    const std::function<std::string_view(std::uint32_t)>& animation_name);
}
}
