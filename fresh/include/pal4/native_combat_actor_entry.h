#pragma once
#include <cstdint>
namespace pal4 {
class NativeCombatActorModelOwner;
class NativeCombatPlayerActorOwner;
class NativeCombatPlayerModelCache;
class NativeCombatPlayerMotion;
class NativeCombatActorSkillRuntime;
class NativePlayerCollectionCatalog;
class NativeEffectSceneSession;
class NativeScriptEventRouter;
class INativeCombatMonsterRoster;
void RefreshNativeCombatActorAttachments(NativeCombatActorModelOwner&,
    NativeCombatActorSkillRuntime&,const NativePlayerCollectionCatalog&,
    NativeEffectSceneSession&,NativeScriptEventRouter&,INativeCombatMonsterRoster&);
// Borrows services for shared52CFD0. Caller pins the manager/model for the
// whole sequence; individual skill calls retain their existing dispatch guards.
void InitializeNativeCombatActor(NativeCombatActorModelOwner&,
    NativeCombatActorSkillRuntime&,const NativePlayerCollectionCatalog&,
    NativeEffectSceneSession&,NativeScriptEventRouter&,INativeCombatMonsterRoster&,
    std::uint32_t slot);
// Player uses5394E0 stage entry,5395E0 motion,538BC0 stats and537AC0
// virtual256/value1196. All model/skill/world/effect services must be bound.
void InitializeNativeCombatActor(NativeCombatPlayerActorOwner&,NativeCombatPlayerModelCache&,
    NativeCombatActorSkillRuntime&,const NativePlayerCollectionCatalog&,
    NativeEffectSceneSession&,NativeScriptEventRouter&,NativeCombatPlayerMotion&,
    std::uint32_t slot);

}
