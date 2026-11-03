#pragma once
#include "pal4/combat_actor_status.h"
#include <memory>
#include <span>

namespace pal4 {
struct ScriptForwardResult;
class NativeCombatActorOwner;
class NativeCombatPlayerActorOwner;
class NativeScriptEventRouter;
class IScriptSkillCollectionRuntime;
class INativeCombatActorTurnServices;

// Borrows the actor, synchronous event router and real skill services. Binds
// Player/Monster health virtual dispatch through the existing base damage property.
// Owns no nodes: the actor allocator/owned chain retains base and supported status nodes.
class NativeCombatActorHealthRuntime final : public ICombatActorStatusRuntime {
public:
    NativeCombatActorHealthRuntime(NativeCombatActorOwner&, NativeScriptEventRouter&,
                                   IScriptSkillCollectionRuntime&);
    NativeCombatActorHealthRuntime(NativeCombatPlayerActorOwner&, NativeScriptEventRouter&,
                                   IScriptSkillCollectionRuntime&);
    ~NativeCombatActorHealthRuntime();
    NativeCombatActorHealthRuntime(const NativeCombatActorHealthRuntime&) = delete;
    NativeCombatActorHealthRuntime& operator=(const NativeCombatActorHealthRuntime&) = delete;
    [[nodiscard]] std::int32_t Update();
    [[nodiscard]] std::int32_t SetCurrent(std::uint32_t index,std::int32_t value);
    // Actual actor68/property168 dispatch, preserving the current property chain.
    [[nodiscard]] std::int32_t SetCurrentViaActor(std::uint32_t index,std::int32_t value);
    // Actor64 ->52F2F0 -> property164 ->56C320/56CE40. Base parameters0..7,
    // distinct from current HP/SP/MP actor68 and the permanent Player fields.
    [[nodiscard]] std::int32_t SetParameterViaActor(std::uint32_t index,std::int32_t value);
    // Actor32/property132: element modifier restoration during581690 growth.
    [[nodiscard]] std::int32_t SetElementViaActor(std::uint32_t index,std::int32_t value);
    // Actor84 -> property184 ->52/68; signed 32-bit sum then real validation/events.
    [[nodiscard]] std::int32_t AddCurrent(std::uint32_t index,std::int32_t delta);
    // IDA56E2F0, property800 slot4; distinct from attack slot8.
    [[nodiscard]] bool AllowsAction();
    ScriptForwardResult ForwardPropertyNotification(std::uint32_t property,std::uint32_t slot,
        std::span<const std::uint32_t> arguments);
    // Original52B4F0 command0..5 through actual property800 slot8.
    [[nodiscard]] std::uint8_t CommandAvailability(std::uint32_t command);
    [[nodiscard]] bool AllowsAttack();
    [[nodiscard]] bool AllowsDamage(std::uint32_t action=0u);
    // Actor56 -> property156 ->56CA60; preserves actual float-change events.
    void SetScalar(std::uint32_t index,std::uint32_t bits);
    // Actor124/128 -> base property224/228 ->56C280/56CAE0.
    [[nodiscard]] float RaceAttack(std::uint32_t index);
    void SetRaceAttack(std::uint32_t index,std::uint32_t float_bits);
    // The original null-link property's return register is incidental;548
    // callers use the state/event effects, so no native success token is exposed.
    void BecomeAlive();
    // Actual532/52C120: state6, ae_fleed and base property80 forwarding.
    void Fled();
    void BeginFlee();
    void FleeFailed();
    // Actor540/52C1A0: ae_defende then actual property84 forwarding.
    void Defend();
    // Player death requires the concrete native skill runtime cleanup binding.
    // Actual544/52C1E0. Skill services supply584 cleanup and276 rebuild;
    // a loaded model also requires its real animation/geometry queue services.
    void BecomeDead(std::uint32_t cause,INativeCombatActorTurnServices* = nullptr);
    [[nodiscard]] bool IsReady();
    [[nodiscard]] bool IsUnavailable();
    // IDA56E350: property344 broadcast (base property5A6010 is a no-op).
    void BroadcastProperties();
    //589970 includes52EB40 alpha restoration when actor808 has a model.
    void ResetScriptProperties(INativeCombatActorTurnServices* visuals=nullptr);
    // IDA52C350: state at828, optional ae_state_changed notification.
    void SetFacetState(std::uint32_t state,bool notify);
    // Actor192/196 -> base property292/296: action-state660, range0..2.
    // This is distinct from the facet readiness state at actor828.
    std::int32_t Query(std::uint32_t actor,std::uint32_t slot) override;
    std::int32_t QueryIndexed(std::uint32_t actor,std::uint32_t slot,std::uint32_t index) override;
    // Actual actor40/100/112/124 through their base damage property getters.
    [[nodiscard]] double QueryFloat(std::uint32_t actor,std::uint32_t slot,std::uint32_t index);
    std::int32_t Set(std::uint32_t actor,std::uint32_t slot,std::int32_t value) override;
private:
    friend class NativeCombatActorSkillRuntime;
    void EnablePlayerDeathCleanup();
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace pal4
