#pragma once
#include <span>
#include "pal4/native_combat_property_action_runtime.h"
namespace pal4 {
class NativeCombatSkillActorRegistry;class NativeInventorySession;class NativeCombatRosterTargets;class NativeScriptEventRouter;
class NativeSkillRecordCatalog;class NativeCombatActorSkillRuntime;class NativeCombatHints;class NativeGameTimerQueue;
class NativeActorInstanceOwner;class ICombatActionTaskRuntime;
class NativeCombatCommandActions;class NativeCombatEventHandlers;class NativeCombatPropertyDomain;
class NativeCombatTaskQueue;class NativeCombatPlayerActionMemory;class NativeCombatDeathActions;
namespace test {
std::uint32_t FindNativeCombatPropertyForUi(NativeInventorySession&,NativeSkillRecordCatalog&);
void VerifyNativeCombatPropertyEffects(NativeCombatSkillActorRegistry&,NativeInventorySession&,NativeCombatRosterTargets&,NativeScriptEventRouter&,
    NativeSkillRecordCatalog&,std::span<NativeCombatActorSkillRuntime* const>,NativeCombatHints&,NativeGameTimerQueue&);
void VerifyNativeCombatPropertyAnimation(NativeCombatSkillActorRegistry&,NativeInventorySession&,NativeCombatRosterTargets&,
    NativeScriptEventRouter&,NativeCombatActorSkillRuntime&,NativeCombatActorSkillRuntime&,NativeCombatMessageChildren::Model,
    NativeEffectSceneSession&,ICombatActionTaskRuntime&,float&,NativeCombatPropertyActionRuntime::Motion);
void VerifyNativeCombatPropertyCommand(NativeCombatCommandActions&,NativeCombatEventHandlers&,NativeInventorySession&,
    NativeCombatActorSkillRuntime&,NativeCombatActorSkillRuntime&,NativeCombatPropertyDomain&,NativeCombatTaskQueue&,
    NativeCombatPlayerActionMemory&,NativeScriptEventRouter&,NativeCombatDeathActions&,NativeCombatMessageChildren::Model,
    NativeCombatPropertyActionRuntime::Motion,const std::function<void()>&,const std::function<void()>& start_ui={},std::uint32_t property_id=3012u,bool revive_target=false,bool attack_target=false);
}
}
