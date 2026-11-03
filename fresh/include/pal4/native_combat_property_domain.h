#pragma once
#include "pal4/native_combat_task_domain.h"
#include "pal4/native_combat_property_action_runtime.h"
#include "pal4/native_combat_property_runtime.h"
#include "pal4/native_combat_throw_property.h"
#include <memory>
#include <functional>
namespace pal4 {
class NativeCombatSkillActorRegistry;class NativeInventorySession;class NativeCombatRosterTargets;
class NativeCombatDeathActions;struct NativeCombatSystemImage;
class NativeCombatActionChildren;
struct NativeCombatPropertyServices {
    std::function<NativeActorInstanceOwner&(std::uint32_t)> model;
    NativeCombatPropertyActionRuntime::Motion motion;
    std::function<NativeCombatDeathActions&(std::uint32_t)> death;
    std::function<std::uint32_t(std::uint32_t)> register_task;
    NativeCombatPropertyTargetServices target_effects;
    NativeCombatThrowPropertyServices projectile;
};
// Per-source original UseProperty/EndAction parents. Queue and replay owners
// retire first. All actual actor, inventory, model and effect services outlive it.
class NativeCombatPropertyDomain final:public INativeCombatTaskDomain {
public:
    NativeCombatPropertyDomain(std::uint32_t,NativeCombatSkillActorRegistry&,NativeCombatSystemImage&,
        NativeInventorySession&,NativeCombatRosterTargets&,NativeEffectSceneSession&,NativeScriptEventRouter&,
        ICombatActionTaskRuntime&,NativeCombatActionChildren&,float&,NativeCombatPropertyServices);
    ~NativeCombatPropertyDomain();
    std::uint32_t Create(std::uint32_t record,std::uint32_t target);
    std::uint32_t ActorAddress()const noexcept;
    bool Owns(std::uint32_t)const noexcept override;bool Ready(std::uint32_t)const override;
    bool Enabled(std::uint32_t)const override;void SetEnabled(std::uint32_t,bool)override;
    void DisableForReset(std::uint32_t)override;
    std::uint32_t Identity(std::uint32_t)const override;bool Processed(std::uint32_t)const override;
    void Prepare(std::uint32_t)override;void BeginWaiting(std::uint32_t)override;
    void Update(std::uint32_t,float)override;void Stop(std::uint32_t)override;
    std::uint32_t ResultEvent(std::uint32_t)override;void Cancel(std::uint32_t)override;
    void Close();std::size_t Count()const noexcept;std::size_t OwnedAllocations()const noexcept;
    std::size_t OwnedTexts()const noexcept;std::size_t Subscriptions()const noexcept;
    std::size_t Throws()const noexcept;std::size_t ProjectileUpdates()const noexcept;
    std::size_t HiddenProjectilesOutsideWorld()const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
