#pragma once
#include "pal4/native_combat_task_domain.h"
#include "pal4/native_combat_skill_action_runtime.h"
#include <functional>
#include <memory>
namespace pal4 {
class NativeActorInstanceOwner;struct NativeCombatSystemImage;class NativeCombatActionChildren;
struct NativeCombatSkillServices {
    std::function<NativeActorInstanceOwner&(std::uint32_t)> model;
    NativeCombatSkillActionRuntime::EffectFactory effect;
    NativeCombatSkillActionRuntime::Motion motion;
    NativeCombatSkillActionRuntime::Wounded wounded;
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> dead;
    std::function<std::uint32_t(std::uint32_t)> register_task;
    NativeCombatSkillActionRuntime::ResetModel reset_model;
    std::function<std::array<float,3>(std::uint32_t)> formation;
    std::function<void(std::uint32_t,std::uint32_t,INativeCombatTaskDomain&)> attach_dead;
    std::function<void(std::uint32_t)> cancel_dead;
};
// Per-source Magic/Stunt sequence parents and mixed message/position children.
// Queue registrations retire before this owner; actors/records/models outlive it.
class NativeCombatSkillDomain final:public INativeCombatTaskDomain {
public:
    NativeCombatSkillDomain(std::uint32_t,NativeCombatSkillActorRegistry&,NativeCombatSystemImage&,
        const NativeMagicWindowRecords&,const NativeCombatFormulaCatalog&,NativeCombatDamageCalculator&,
        NativeCombatRosterTargets&,NativeEffectSceneSession&,NativeScriptEventRouter&,ICombatActionTaskRuntime&,
        NativeCombatActionChildren&,float&,NativeCombatSkillServices,
        NativeCombatSkillKind kind=NativeCombatSkillKind::magic);
    ~NativeCombatSkillDomain();
    std::uint32_t Create(std::uint32_t record,std::uint32_t target);
    std::string_view Name(std::uint32_t)const;
    std::uint32_t ActorAddress()const noexcept;
    NativeCombatSkillKind Kind()const noexcept;
    bool Owns(std::uint32_t)const noexcept override;bool Ready(std::uint32_t)const override;
    bool Enabled(std::uint32_t)const override;void SetEnabled(std::uint32_t,bool)override;
    void DisableForReset(std::uint32_t)override;
    std::uint32_t Identity(std::uint32_t)const override;bool Processed(std::uint32_t)const override;
    void Prepare(std::uint32_t)override;void BeginWaiting(std::uint32_t)override;
    void Update(std::uint32_t,float)override;void Stop(std::uint32_t)override;
    std::uint32_t ResultEvent(std::uint32_t)override;void Cancel(std::uint32_t)override;
    void Close();std::size_t Count()const noexcept;std::size_t OwnedAllocations()const noexcept;
    std::size_t OwnedTexts()const noexcept;std::size_t Subscriptions()const noexcept;std::size_t Casts()const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
