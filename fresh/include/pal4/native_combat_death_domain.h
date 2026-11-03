#pragma once
#include <cstdint>
#include <memory>
namespace pal4 {
class NativeCombatPlayerModelCache;
class NativeCombatActorModelOwner;
class NativeCombatActorSkillRuntime;
class NativeCombatActorTurnRuntime;
class INativeSkillSystemServices;
class NativePlayerCollectionCatalog;
class NativeScriptEventRouter;
class NativeEffectSceneSession;
class NativeCombatDeathActions;
struct NativeCombatSystemImage;
// Real per-actor Dead/QuickDead composition. All referenced actor/model/skill/
// effect/turn services outlive this domain. Clear the shared battle queue before
// destroying domains, and destroy domains before retiring their dependencies.
class NativeCombatDeathDomain final {
public:
    NativeCombatDeathDomain(NativeCombatPlayerModelCache&,NativeCombatActorSkillRuntime&,
        NativeCombatActorTurnRuntime&,NativeCombatSystemImage&,INativeSkillSystemServices&,
        const NativePlayerCollectionCatalog&,NativeScriptEventRouter&);
    NativeCombatDeathDomain(NativeCombatActorModelOwner&,NativeCombatActorSkillRuntime&,
        NativeCombatActorTurnRuntime&,NativeCombatSystemImage&,INativeSkillSystemServices&,
        const NativePlayerCollectionCatalog&,NativeEffectSceneSession&);
    ~NativeCombatDeathDomain();
    NativeCombatDeathDomain(const NativeCombatDeathDomain&)=delete;
    NativeCombatDeathDomain& operator=(const NativeCombatDeathDomain&)=delete;
    NativeCombatDeathActions& Actions();
    std::uint32_t ActorAddress() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
