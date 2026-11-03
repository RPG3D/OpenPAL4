#pragma once
#include <cstdint>
#include <functional>
namespace pal4 {
class NativeCombatActorOwner;
class NativeCombatPlayerActorOwner;
class NativeCombatActorSkillRuntime;
class IEffectHandleContentRuntime;
// Original shared virtual388 (52DE40). The caller pins the manager and supplies
// actual virtual576 (Player cache LeaveStage / Monster LeaveStageAndScene).
// Uses the existing dynamic-effect tree owner and shared release algorithm.
// This is not actor
// destruction, combat-over, or full roster clearing.
std::uint32_t RetireNativeCombatActor(NativeCombatActorOwner&,
    IEffectHandleContentRuntime&,const std::function<void()>& leave_stage);
std::uint32_t RetireNativeCombatActor(NativeCombatPlayerActorOwner&,
    IEffectHandleContentRuntime&,const std::function<void()>& leave_stage);
//Actor ownership stays pinned by its actual544/532 dispatcher. Only Monster388
//payload retirement is authorized here; destruction still requires idle dispatch.
std::uint32_t RetireNativeCombatActorDuringExit(NativeCombatActorOwner&,NativeCombatActorSkillRuntime&,
    IEffectHandleContentRuntime&,const std::function<void()>& leave_stage);
// Only the matching9126 candidate388 callback may retire during skill dispatch.
std::uint32_t RetireNativeCombatActorDuringSkill(NativeCombatActorOwner&,NativeCombatActorSkillRuntime&,
    IEffectHandleContentRuntime&,const std::function<void()>& leave_stage);
std::uint32_t RetireNativeCombatActorDuringSkill(NativeCombatPlayerActorOwner&,NativeCombatActorSkillRuntime&,
    IEffectHandleContentRuntime&,const std::function<void()>& leave_stage);
//Selected Player may differ from the actual544 source (57B220 slot0 fallback).
std::uint32_t RetireNativeCombatPlayerDuringReplacement(NativeCombatPlayerActorOwner&,NativeCombatActorSkillRuntime&,
    IEffectHandleContentRuntime&,const std::function<void()>& leave_stage);
}
