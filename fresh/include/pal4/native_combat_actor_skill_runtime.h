#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include "pal4/native_combat_skill_effect_services.h"

namespace pal4 {
class NativeMonsterRecordCatalog;
class NativeCombatActorOwner;
class NativeCombatPlayerActorOwner;
class NativeInventorySession;
class NativeCombatFormulaCatalog;
class NativePlayerCollectionCatalog;
class INativeCombatActorModelServices;
class INativeCombatMonsterRoster;
class NativeCombatSkillActorRegistry;
class NativeScriptEventRouter;
class NativeSkillRecordCatalog;
class INativeSkillSystemServices;
class INativeCombatActorTurnServices;
class ICombatEventDataRuntime;
class NativeStuntLearningCatalog;
class NativeMagicWindowRecords;

// Borrows actor/catalog/router/SkillSystem. Owns the actor's two skill lists
// using its allocator; Close must precede actor destruction. AddRecord binds
// shared589170 directly, not Monster virtual268 (the separate52C860 action).
// Registered Player and Monster actors share a routing domain; null contexts remain valid.
// Original
// scalar/indexed effects, recovery9118 and explicitly bound damage callbacks
// execute through actual actor services; other effects require their consumers.
class NativeCombatActorSkillRuntime final {
public:
    NativeCombatActorSkillRuntime(NativeCombatActorOwner&,NativeScriptEventRouter&,
                                 NativeSkillRecordCatalog&,INativeSkillSystemServices&);
    NativeCombatActorSkillRuntime(NativeCombatActorOwner&,NativeCombatSkillActorRegistry&);
    // Player uses the same skill algorithms/notifications, but538BC0 stat
    // refresh and passive cleanup additionally borrow actual inventory. Player
    // turns require INativeCombatPlayerTurnServices; modeled stage stays separate.
    NativeCombatActorSkillRuntime(NativeCombatPlayerActorOwner&,NativeCombatSkillActorRegistry&,NativeInventorySession&);
    ~NativeCombatActorSkillRuntime();
    [[nodiscard]] std::uint32_t ActorAddress() const noexcept;
    NativeCombatActorSkillRuntime(const NativeCombatActorSkillRuntime&)=delete;
    NativeCombatActorSkillRuntime& operator=(const NativeCombatActorSkillRuntime&)=delete;
    bool AddRecord(std::uint32_t id,std::uint32_t context);
    // Actual Monster268/52C860. The target is stored as context; initial effect
    // applies to this actor. The single-argument overload selects self.
    bool ApplyAction(std::uint32_t id);
    bool ApplyAction(std::uint32_t id,std::uint32_t target);
    void Process(std::uint32_t trigger);
    void Process(std::uint32_t trigger,std::uint32_t target);
    // Actual5897F0, immediate target effect without list insertion.
    bool TriggerForTarget(std::uint32_t id,std::uint32_t target);
    void Remove(std::uint32_t id);
    void Advance(std::uint32_t elapsed);
    std::int32_t RebuildStats();
    // Actual53A440 ->581690 after a Player level change. Uses real UpgradeData,
    // preserves five modifiers, rebuilds inventory/skills and restores HP/MP.
    // This neither grants XP nor learns Stunts or shows the ending UI.
    std::int32_t RefreshPlayerGrowth(ICombatEventDataRuntime&);
    // Actual53A460/430640 selection and mutation of this Player's host-owned
    // Stunt collection. Returns stable IDs in original catalog order; scoped
    // logical record handles never escape to native reports or UI.
    std::vector<std::uint32_t> LearnPlayerStunts(const NativeStuntLearningCatalog&);
    //53A590/440350 on this Player's original base property at actor+4.
    std::int32_t IndexedPlayerStat(std::uint32_t index);
    void SetIndexedPlayerStat(std::uint32_t index,std::int32_t value);
    std::int32_t SetCurrentStat(std::uint32_t index,std::int32_t value);
    // Actor68, including active property dispatch (distinct from the base setter).
    std::int32_t SetCurrentStatViaActor(std::uint32_t index,std::int32_t value);
    [[nodiscard]] std::int32_t AddCurrentStat(std::uint32_t index,std::int32_t delta);
    [[nodiscard]] bool AllowsAction();
    [[nodiscard]] std::uint8_t CommandAvailability(std::uint32_t command);
    // Player first five actor920 mastered-magic arrays, shared4301A0.
    [[nodiscard]] bool HasMasteredMagic();
    [[nodiscard]] std::vector<std::uint32_t> MasteredMagic(std::uint32_t element);
    [[nodiscard]] std::vector<std::uint32_t> LearnedPlayerStunts();
    // Actual Player virtual436/5384E0, borrowing a catalog-owned208-byte Stunt.
    [[nodiscard]] bool CanUsePlayerStunt(std::uint32_t,const NativeMagicWindowRecords&);
    [[nodiscard]] bool AllowsAttack();
    [[nodiscard]] bool AllowsDamage(std::uint32_t action=0u);
    [[nodiscard]] bool BlocksReflectedDamage();
    // Actor virtual52 through the real health/property chain (HP7/MP9 etc.).
    [[nodiscard]] std::int32_t CurrentStat(std::uint32_t index);
    // Actor virtual48: effective parameters; maximum HP0 and maximum MP2.
    [[nodiscard]] std::int32_t StatParameter(std::uint32_t index);
    // Read-only original actor28/88 and40/100/112/124 damage operands.
    [[nodiscard]] std::int32_t ElementStat(std::uint32_t index);
    [[nodiscard]] std::int32_t AttackBonus(std::uint32_t index);
    [[nodiscard]] double AttackFactor(std::uint32_t mode);
    [[nodiscard]] double ReflectedDamageFactor(std::uint32_t mode);
    [[nodiscard]] double DamageReduction(std::uint32_t mode);
    [[nodiscard]] double RaceAttackFactor(std::uint32_t race);
    //5894C0/589520 iterators and588F60: record64 status icon IDs, in list order.
    [[nodiscard]] std::vector<std::int32_t> StatusIconIds(bool permanent);
    [[nodiscard]] bool ReadyForAction();
    // Actual actor12/property112, actor252 and owned AI facet respectively.
    [[nodiscard]] std::uint32_t AiType();
    [[nodiscard]] std::int32_t AiTargetId();
    [[nodiscard]] std::int32_t TurnCount();
    [[nodiscard]] std::uint32_t AiTaskAddress();
    // Actual virtual144/property244 (636), virtual168/property268 (648).
    // Queries only; these do not refresh stats or mutate a mode.
    // Actor136/property236 through the existing actual property chain.
    [[nodiscard]] std::int32_t ElementKind();
    [[nodiscard]] std::uint32_t PrimaryMotionState();
    [[nodiscard]] std::uint32_t SecondaryMotionState();
    // Actual actor152/property252 (640), including the active property chain.
    [[nodiscard]] std::uint32_t CombatMode();
    [[nodiscard]] std::uint32_t AttackMode();
    [[nodiscard]] bool IsPlayerActor();
    [[nodiscard]] std::string SoundName(std::uint32_t selector);
    // Actor192 through its real base/linked property chain, action-state660.
    [[nodiscard]] std::uint32_t ActionState();
    // Borrow formulas until Close/rebind. Real property services and actor
    // guards are reused; action selection/execution remain separate.
    void BindActionProgress(const NativeCombatFormulaCatalog&);
    void BindEffectServices(NativeCombatSkillEffectServices);
    using DamageAction = std::function<void(std::uint32_t callback,std::uint32_t record,std::uint32_t source,std::uint32_t target)>;
    void BindDamageActions(DamageAction);
    using RelatedTargets = std::function<std::vector<std::uint32_t>(std::uint32_t)>;
    // Original Monster348/52CC40; Player348 remains the original empty base.
    void BindRelatedTargets(RelatedTargets);
    // Actor352/52CF10: own first roster, without348's element relation filter.
    void BindAllTargets(RelatedTargets);
    // Actual9126 candidate388. Borrows payload/stage retirement services;
    // never grants destruction or death/replacement dispatch permission.
    void BindConditionalRetirement(std::function<void()>);
    [[nodiscard]] bool ConditionalRetirementActive() const noexcept;
    // Payload retirement selected by57B420 during another Player's544.
    [[nodiscard]] bool ReplacementRetirementActive() const noexcept;
    [[nodiscard]] std::uint32_t FundamentalElement();
    void NotifyImmunityNormal();
    [[nodiscard]] float ActionRate();
    [[nodiscard]] float ActionProgress();
    [[nodiscard]] bool ActionProgressReady();
    void PrepareActionProgress();
    void SetActionProgress(float value);
    //476/52BD30 and480/52BD70: real facet events and property28/32.
    // End state2 additionally requires actual Player/Monster motion refresh.
    using ActionMotionRefresh=std::function<void(std::uint32_t actor,std::uint32_t flag)>;
    void StartAction();
    //52C1A0: actor540 event/property.
    void Defend();
    // Actual528/532/536 state/event/property dispatch; no fabricated outcome.
    void BeginFlee();
    void Fled();
    void FleeFailed();
    // Monster5309C0 actor1144>0, Player shared ReturnFalse virtual412.
    [[nodiscard]] bool BlocksPartyEscape();
    void SetActionState(std::uint32_t state);
    //589970: eight property mode setters, through the existing guarded runtime.
    // Loaded models require their actual animation/geometry provider for alpha.
    void ResetScriptProperties(INativeCombatActorTurnServices* visuals=nullptr);
    void EndAction(const ActionMotionRefresh&);
    //552/52C290 and556/52C2D0: synchronous original pair-event records.
    // Nonzero participants must belong to this actual skill routing domain.
    [[nodiscard]] std::uint32_t ProtectAction(std::uint32_t protected_actor,std::uint32_t other);
    [[nodiscard]] std::uint32_t JointAttack(std::uint32_t other);
    [[nodiscard]] std::uint32_t ReactAttack(std::uint32_t other);
    // Actor500/504 notifications and actor484 received-attack property forward.
    void NotifyPhysicalAttack(std::uint32_t target);
    void NotifyCriticalAttack(std::uint32_t target);
    void NotifyAttackMissed(std::uint32_t target);
    //508/52BF20: source attack was blocked; event then property60.
    void NotifyFendOffed(std::uint32_t target);
    void NotifyReceivedAttack(std::uint32_t attacker);
    // Actual488/52BE20 and516/52BFC0; caller retains the Magic record.
    void NotifyReceivedMagic(std::uint32_t caster);
    void NotifyCastMagic(std::uint32_t target,std::uint32_t magic);
    // Actor492/52BE40 and520/52C020: original Stunt notifications/property44/72.
    void NotifyReceivedStunt(std::uint32_t source);
    void NotifyStunt(std::uint32_t target,std::uint32_t stunt);
    // Actual524/52C080 and496/52BE60; execution remains with UsePropertyTask.
    void NotifyUseProperty(std::uint32_t target,std::uint32_t property);
    void NotifyReceivedProperty(std::uint32_t source);
    //52BCB0: original cross-type SP8 additions, scaled/truncated before actor84.
    void TransferAttackMetric8(std::uint32_t target,float scale);
    // Actual Monster530F80 or Player538040 through shared52B8D0; synchronous events.
    // Borrows actual Combat services; keeps actor/skill dispatch guards active.
    void CanTakeAction(INativeCombatActorTurnServices&);
    // Actual Monster548/52C250 through the existing health/event runtime.
    void BecomeAlive();
    [[nodiscard]] bool IsNotStateThree();
    [[nodiscard]] bool IsStateThree();
    // IDA52E270: actor828, distinct from action-state660.
    [[nodiscard]] bool IsStateZero();
    [[nodiscard]] bool IsUnavailable();
    void SetFacetState(std::uint32_t state,bool notify);
    // Monster actor172/52ED50, including the original revive-status branch.
    void ApplyFinalHitPropertyAction();
    // Event virtual8 (SET), using the actor's real property receiver.
    void SetCombatEvent(std::uint32_t event,std::uint32_t float_bits);
    // Actual544/52C1E0: reset statuses, restore passive skills, rebuild stats,
    // then state4/death notification. Whole-list mutation cannot nest.
    void BecomeDead(std::uint32_t cause,const NativePlayerCollectionCatalog&,
                    INativeCombatActorTurnServices* = nullptr);
    //True only while this receiver's original544 call is on the stack.
    bool DeathDispatchActive() const noexcept;
    bool ExitDispatchActive() const noexcept;
    bool FledDispatchActive() const noexcept;
    bool HasReviveStatus();
    // Actual Monster572/531720 only: restore HP/MP, refresh the matching model,
    // emit join stage/scene. Does not replace full52CFD0 registration/effects.
    void JoinStageAndScene(INativeCombatActorModelServices&);
    void LeaveStageAndScene();
    // One-word me_new_added/re_new_added event; manager inserts before calling.
    void NotifyNewActor();
    // Actual396/5317E0; supplied model must own this actor.
    void BeginMotion(INativeCombatActorModelServices&,INativeCombatMonsterRoster*);
    void UpdateMotion(INativeCombatActorModelServices&,std::uint32_t flag=1u,INativeCombatMonsterRoster* = nullptr);
    // Actual584: Monster531600 restores five actor896 Stunt/Magic IDs;
    // Player539B20 restores equipment, learned Magic and Stunt permanent skills.
    void CleanupObjectLists(const NativePlayerCollectionCatalog&);
    void Close();
    [[nodiscard]] std::size_t OwnedBuffers() const noexcept;
private:
    friend class NativeCombatMonsterManager;
    void ApplyPendingSceneRecord(const NativeMonsterRecordCatalog&,const NativePlayerCollectionCatalog&,
        INativeCombatActorModelServices&,const std::function<void()>& detach,
        const std::function<void()>& refresh_attachments);
    friend class NativeCombatActorExitEvents;
    void InitializeReplacementFrom(NativeCombatActorSkillRuntime& source,const std::function<void()>& initialize);
    void RetireReplacementFrom(NativeCombatActorSkillRuntime& source,const std::function<void()>& retire);
    class Impl;
    std::unique_ptr<NativeCombatSkillActorRegistry> private_registry_;
    std::unique_ptr<Impl> impl_;
};
}
