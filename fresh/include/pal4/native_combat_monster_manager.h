#pragma once
#include "pal4/native_combat_actor_model_services.h"
#include "pal4/native_combat_scene_actors.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>
namespace pal4 {
class NativeActorInstancePool;
class NativeMonsterRecordCatalog;
class NativeSkillRecordCatalog;
class NativePlayerCollectionCatalog;
class NativeScriptEventRouter;
class INativeSkillSystemServices;
class NativeCombatActorModelOwner;
class NativeCombatActorSkillRuntime;
class NativeCombatSkillActorRegistry;
class NativeEffectSceneSession;
class INativeCombatActorTurnServices;
// Owns Monster constructor/model/skill adapters and original first/stage lists.
// Borrows pool/catalogs/router/SkillSystem; Close before those dependencies die.
// Uses native STL storage/cursors, never a VC6 collection image. Creation and
// stage membership and explicit52CFD0 entry are real; Combat state entry and
// original combat-over orchestration remain separate.
class NativeCombatMonsterManager final : public INativeCombatMonsterRoster,public INativeCombatSceneActors {
public:
    NativeCombatMonsterManager(NativeActorInstancePool&,NativeMonsterRecordCatalog&,
        NativeSkillRecordCatalog&,NativeScriptEventRouter&,INativeSkillSystemServices&);
    // Borrow a shared Player/Monster routing domain; it outlives the manager.
    // Close retires only this manager's actors, preserving other registry users.
    NativeCombatMonsterManager(NativeActorInstancePool&,NativeMonsterRecordCatalog&,
        NativeSkillRecordCatalog&,NativeScriptEventRouter&,INativeSkillSystemServices&,
        NativeCombatSkillActorRegistry&);
    ~NativeCombatMonsterManager() override;
    NativeCombatMonsterManager(const NativeCombatMonsterManager&)=delete;
    NativeCombatMonsterManager& operator=(const NativeCombatMonsterManager&)=delete;
    std::uint32_t CreateMonster(std::int32_t id,std::uint32_t slot,std::uint32_t group=0u);
    // Host service ownership is composed after insertion, before the original
    // new-actor notification. Bind only on an empty idle manager. This callback
    // must not replace original game initialization or emit a synthetic event.
    void BindActorPreparation(std::function<void(std::uint32_t actor)>);
    void CleanupObjectLists(std::uint32_t actor,const NativePlayerCollectionCatalog&);
    // Full shared52CFD0 sequence with real model/skill/effect services. Borrows
    // the session; caller must retire actor868/876/884 before model destruction.
    //57B1A0: select an empty actual stage slot then initialize the current
    //new actor synchronously. Requires this manager's me_new_added event.
    //Host status:1 after actual initialization,6 when all formation slots are full;
    //the original initializer's incidental EAX is not exposed by its void adapter.
    std::uint8_t HandleNewActor(std::uint32_t payload,
        const NativePlayerCollectionCatalog&,NativeEffectSceneSession&);
    void InitializeMonster(std::uint32_t actor,std::uint32_t slot,
        const NativePlayerCollectionCatalog&,NativeEffectSceneSession&);
    // Native shutdown/request replacement: remove borrowed hook frames and
    // force-release entry attachments before the child/model pool is retired.
    // This does not implement original combat-over/slot388.
    void RetireEntryEffects(std::uint32_t actor,NativeEffectSceneSession&);
    // Original virtual388: effects -> state832 -> leave stage/scene -> slot6.
    // Keeps first-list ownership; full roster clearing/destruction is separate.
    std::uint32_t RetireActor(std::uint32_t actor,NativeEffectSceneSession&);
    //57B220 may retire payload/stage membership inside actual544 while first
    //roster cursors pin owners. Live stage cursors and nested retirements reject.
    std::uint32_t RetireActorDuringExit(std::uint32_t actor,NativeEffectSceneSession&);
    std::uint32_t RetireActorDuringSkill(std::uint32_t actor,NativeEffectSceneSession&);
    void InitializeReplacementDuringExit(std::uint32_t source,std::uint32_t candidate,std::uint32_t slot,
        const NativePlayerCollectionCatalog&,NativeEffectSceneSession&);
    //57B520: restore actor444 position then actual396(1) motion.
    void HandleBecomeAlive(std::uint32_t actor);
    void JoinStageAndScene(std::uint32_t actor);
    void LeaveStageAndScene(std::uint32_t actor);
    void BeginMonsterMotion(std::uint32_t actor);
    void UpdateMonsterMotion(std::uint32_t actor,std::uint32_t flag=1u) override;
    // Keep manager ownership stable during synchronous turn/skill/router calls.
    void CanTakeAction(std::uint32_t actor,INativeCombatActorTurnServices&);
    //531F40, callable only by this actor's actual pending-record turn callback.
    void ApplyPendingSceneRecord(std::uint32_t actor,const NativePlayerCollectionCatalog&,NativeEffectSceneSession&);
    // Owns the real visual selection adapter until Close. The effect session
    // and its groups must outlive this manager and its returned model effects.
    std::uint32_t SetSelected(std::uint32_t actor,std::uint32_t selected,NativeEffectSceneSession&);
    // Monster half of529AC0/529B10: first-list order, null children skipped.
    // PlayerManager traversal remains the enclosing ActorManager's obligation.
    void RenderModels(std::uint32_t camera,std::uint32_t slot);
    //53B3E0 first loop only: stage-list child56; actor728 list follows separately.
    void UpdateStageModels(float elapsed);
    // Complete Monster53B3E0: stage child56, then first-list actor728 (also
    // offstage actors). Populated queues require real bound activity services.
    void UpdateActors(float elapsed);
    //529B70 first roster, including offstage owners; retains collection guard.
    void DispatchAnimations(std::uint32_t context);
    void BindSceneEvents() override;
    void UnbindSceneEvents() noexcept override;
    std::uint32_t SceneChild(std::uint32_t actor) override;
    void ActivateSceneModel(std::uint32_t actor,std::uint32_t world,std::uint32_t queue) override;
    void PositionSceneModel(std::uint32_t actor,const ResourceVec3&) override;
    void RestoreSceneCallbacks() override;
    [[nodiscard]] NativeCombatActorModelOwner& ModelOwner(std::uint32_t actor);
    [[nodiscard]] NativeCombatActorSkillRuntime& Skills(std::uint32_t actor);
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t StageCount() const noexcept;
    [[nodiscard]] std::size_t SubscriptionCount() const;
    [[nodiscard]] std::uint32_t ActorAt(std::size_t index) const;
    [[nodiscard]] std::uint32_t StageActorAt(std::size_t index) const;
    // Original first roster, distinct from the stage list. Cursors protect
    // both creation/retirement and reject handles from another cursor family.
    std::uint32_t CreateFirstIterator();
    bool FirstIteratorAtEnd(std::uint32_t);
    std::uint32_t FirstIteratorActor(std::uint32_t);
    void NextFirstIterator(std::uint32_t);
    void DeleteFirstIterator(std::uint32_t);
    [[nodiscard]] std::size_t ActiveIterators() const noexcept;
    //533820 two-pass clear with real virtual388 and native model/skill owners.
    //Retains subscriptions, registry and scene binding for the next battle.
    //Retry after failure before creating/updating actors again.
    void ClearActors(NativeEffectSceneSession&);
    // Native owner teardown only; not original combat-over/slot388 orchestration.
    void Close();
    std::uint32_t CreateStageIterator() override;
    std::uint32_t CloneStageIterator(std::uint32_t);
    bool StageIteratorAtEnd(std::uint32_t) override;
    std::uint32_t StageIteratorActor(std::uint32_t) override;
    void NextStageIterator(std::uint32_t) override;
    void ResetStageIterator(std::uint32_t);
    void DeleteStageIterator(std::uint32_t) override;
    std::string_view MonsterAnimationName(std::uint32_t child_animation) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
