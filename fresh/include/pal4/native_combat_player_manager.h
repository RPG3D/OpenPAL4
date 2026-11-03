#pragma once
#include <cstdint>
#include <cstddef>
#include <memory>
#include <functional>
namespace pal4 {
class NativePlayerManagerSession;
class NativeScriptEventRouter;
class INativeSkillSystemServices;
class NativeCombatPlayerActorOwner;
class IEffectHandleReleaseRuntime;
// Owns Player combat actors and first/stage lists. Borrows the real source
// session, router and initialized SkillSystem, all of which must outlive it.
// Native Close is not original combat-over orchestration or outcome writeback.
class NativeCombatPlayerManager final {
public:
    NativeCombatPlayerManager(NativePlayerManagerSession&,NativeScriptEventRouter&,INativeSkillSystemServices&);
    ~NativeCombatPlayerManager();
    // Rebind before stage creation to preserve original same-mode event order:
    // roster insertion precedes SystemManager527FF0's readiness check.
    // Native creating owners may exist, but no dispatch/cursor/stage may be live.
    void Subscribe();
    // Host service ownership is composed after insertion, before the original
    // new-actor notification. Bind only on an empty idle manager. This callback
    // must not replace original game initialization or emit a synthetic event.
    void BindActorPreparation(std::function<void(std::uint32_t actor)>);
    std::uint32_t AddPlayer(std::uint32_t source_index,std::uint32_t state=6u);
    //5756D0 active-player queue: scan four, filterB08, sort4419D0,
    //pre-step actual priority getter, use state1 for one or6 for multiple.
    //Requires an empty first list. This does not run full Combat Entry.
    void CreateActiveParty();
    // Actual57B120: current re_new_added only, real stage occupancy and three
    // slots. initialize binds complete52CFD0 services synchronously. Full
    // roster returns3 without invoking it; successful host result is1.
    std::uint8_t HandleNewActor(std::uint32_t payload,
        const std::function<void(std::uint32_t actor,std::uint32_t slot)>& initialize);
    NativeCombatPlayerActorOwner& Actor(std::uint32_t);
    std::uint32_t ActorAt(std::size_t) const;
    std::uint32_t StageActorAt(std::size_t) const;
    std::size_t Count() const noexcept;
    std::size_t StageCount() const noexcept;
    std::size_t SubscriptionCount() const;
    std::uint32_t CreateStageIterator();
    std::uint32_t CloneStageIterator(std::uint32_t);
    bool StageIteratorAtEnd(std::uint32_t);
    std::uint32_t StageIteratorActor(std::uint32_t);
    void NextStageIterator(std::uint32_t);
    void ResetStageIterator(std::uint32_t);
    void DeleteStageIterator(std::uint32_t);
    // Original first roster, distinct from the stage list. Cursors protect
    // both creation/retirement and reject handles from another cursor family.
    std::uint32_t CreateFirstIterator();
    bool FirstIteratorAtEnd(std::uint32_t);
    std::uint32_t FirstIteratorActor(std::uint32_t);
    void NextFirstIterator(std::uint32_t);
    void DeleteFirstIterator(std::uint32_t);
    std::size_t ActiveIterators() const noexcept;
    //529B70 first roster: preserves owners across activity virtual8 callbacks.
    void DispatchAnimations(std::uint32_t context);
    //53B450: all virtual388 retirements precede owner release. The caller
    //supplies real model/skill detachment; subscriptions/capacity stay alive.
    //On callback failure retry with the same services; creation is blocked
    //until ClearActors succeeds. Completed phases are not repeated.
    // Optional shutdown context permits only handles of the actual unregistered
    // group; detach must unregister borrowed scene caches before owner release.
    void ClearActors(const std::function<void(std::uint32_t)>& retire,
                     const std::function<void(std::uint32_t)>& detach,
                     std::uint32_t retired_group=0u,IEffectHandleReleaseRuntime* retired_effects=nullptr);
    void Close();
private:
    friend class NativeCombatActorExitEvents;
    //The exit adapter verifies actual544/registry identity before this
    //lower-layer owner scope. Only exhausted stage cursors may survive it.
    void WithExitStageMutation(std::uint32_t dispatcher,std::uint32_t receiver,
        bool join,const std::function<void()>& operation);
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
