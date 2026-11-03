#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatActorModelOwner;
class NativeCombatPlayerModelCache;
class NativeCombatActorSkillRuntime;
class NativeCombatSkillActorRegistry;
struct NativeCombatSystemImage;
class INativeSkillSystemServices;
class NativeEffectSceneSession;
class NativeScriptEventRouter;
class NativeCombatDeathActions;
class NativeCombatFendOffActions;
class NativeCombatMissActions;
class NativeCombatWoundedActions;
class NativeCombatEndSoundEvents;
class NativeCombatActionChildren;
class NativeCombatResultActions;
class ICombatActionTaskRuntime;
// Per-actor reaction services, borrowing the live model, skills, frame scale
// and death domain. Queues detach before this domain; dependencies retire last.
class NativeCombatReactionDomain final {
public:
    using PrepareAction=std::function<void(std::uint32_t actor,std::uint32_t flag)>;
    NativeCombatReactionDomain(NativeCombatActorModelOwner&,NativeCombatActorSkillRuntime&,
        NativeCombatSkillActorRegistry&,NativeCombatSystemImage&,INativeSkillSystemServices&,
        NativeEffectSceneSession&,NativeScriptEventRouter&,float&,NativeCombatDeathActions&,PrepareAction);
    NativeCombatReactionDomain(NativeCombatPlayerModelCache&,NativeCombatActorSkillRuntime&,
        NativeCombatSkillActorRegistry&,NativeCombatSystemImage&,INativeSkillSystemServices&,
        NativeEffectSceneSession&,NativeScriptEventRouter&,float&,NativeCombatDeathActions&,PrepareAction);
    ~NativeCombatReactionDomain();
    NativeCombatReactionDomain(const NativeCombatReactionDomain&)=delete;
    NativeCombatReactionDomain& operator=(const NativeCombatReactionDomain&)=delete;
    NativeCombatFendOffActions& FendOff();
    NativeCombatMissActions& Miss();
    // Borrow existing actor564 animation playback for other action domains.
    ICombatActionTaskRuntime& ActionPlayback();
    // Bind before queue registration; audio/death services outlive this domain.
    void BindWounded(NativeCombatEndSoundEvents&);
    // Monster-only FinalHit services; all borrowed owners outlive this domain.
    void BindFinalHit(NativeCombatActorModelOwner&,NativeEffectSceneSession&,NativeScriptEventRouter&);
    NativeCombatWoundedActions& Wounded();
    void BindResults(NativeCombatActionChildren&);
    NativeCombatResultActions& Results();
    std::uint32_t ActorAddress() const noexcept;
    void Close();
    std::size_t Count() const noexcept;
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
    std::size_t Subscriptions() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
