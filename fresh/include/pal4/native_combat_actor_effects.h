#pragma once
#include "pal4/actor_render_state.h"
#include <memory>

namespace pal4 {
class NativeCombatActorModelOwner;
class NativeCombatPlayerModelCache;
class NativeEffectSceneSession;
class NativeScriptEventRouter;
// Actual52E2E0 for Monster owners or Player model caches. Borrows all services and
// actor/model/facet storage. Retire handles and this adapter before owners die.
class NativeCombatActorEffects final {
public:
    NativeCombatActorEffects(NativeCombatActorModelOwner&,NativeEffectSceneSession&,NativeScriptEventRouter&);
    NativeCombatActorEffects(NativeCombatPlayerModelCache&,NativeEffectSceneSession&,NativeScriptEventRouter&);
    ~NativeCombatActorEffects();
    NativeCombatActorEffects(const NativeCombatActorEffects&)=delete;
    NativeCombatActorEffects& operator=(const NativeCombatActorEffects&)=delete;
    ActorEffectHandle Create(std::string_view resource,std::string_view hook);
    //52F280: actor724 position facet reads the child's live stored position.
    std::array<float,3> ReadPositionFacet(std::uint32_t facet) const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
