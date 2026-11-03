#pragma once
#include "pal4/native_combat_task_domain.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <functional>
#include <array>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class INativeCombatSceneActors;
class NativeCombatProtectionSelection;
class NativeCombatActionChildren;
struct NativeCombatSystemImage;
class NativeCombatPhysicalAttackActions;
class NativeScriptEventRouter;
struct NativeCombatEndAttackServices {
    std::function<void(std::uint32_t)> end_action;
    std::function<bool()> all_ready;
    std::function<std::uint32_t()> random;
    std::function<std::array<float,3>(std::uint32_t)> position;
    NativeScriptEventRouter* events{};
};
// Real CheckProtect/StartAttack execution and PhysicalAttack construction.
// All borrowed actor/selection/child owners outlive this owner. PhysicalAttack
// execution remains required before binding this domain to gameplay.
class NativeCombatAttackActions final : public INativeCombatTaskDomain {
public:
    using PhysicalSuccessor=std::function<NativeCombatPhysicalAttackActions&(std::uint32_t source)>;
    NativeCombatAttackActions(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        std::span<NativeCombatActorSkillRuntime* const>,INativeCombatSceneActors& players,
        NativeCombatProtectionSelection&,NativeCombatActionChildren&,NativeCombatSystemImage&,PhysicalSuccessor={},NativeCombatEndAttackServices={});
    ~NativeCombatAttackActions();
    NativeCombatAttackActions(const NativeCombatAttackActions&)=delete;
    NativeCombatAttackActions& operator=(const NativeCombatAttackActions&)=delete;
    std::uint32_t Create(std::uint32_t source,std::uint32_t target,std::uint8_t mode);
    std::uint32_t CreateEndSuccessor(std::uint32_t physical_predecessor);
    bool Owns(std::uint32_t) const noexcept override;
    bool Ready(std::uint32_t) const override;
    bool Enabled(std::uint32_t) const override;
    void SetEnabled(std::uint32_t,bool) override;
    std::uint32_t Identity(std::uint32_t) const override;
    bool Processed(std::uint32_t) const override;
    void Prepare(std::uint32_t) override;
    void BeginWaiting(std::uint32_t) override;
    void Update(std::uint32_t,float) override;
    void Stop(std::uint32_t) override;
    std::uint32_t ResultEvent(std::uint32_t) override;
    void Cancel(std::uint32_t) override;
    void Close();
    std::size_t Count() const noexcept;
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
