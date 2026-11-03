#pragma once
#include "pal4/native_combat_task_domain.h"
#include <cstddef>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatSkillActorRegistry;
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatRosterTargets;
class NativeCombatDamageCalculator;
class ICombatActionTaskRuntime;
struct NativeCombatSystemImage;
// Original Flee/QuickFlee recipes with shared GOM/status/animation owners.
// Success emits real ae_fleed; the complete outcome consumer is still required.
class NativeCombatFleeActions final : public INativeCombatTaskDomain {
public:
    using Playback=std::function<ICombatActionTaskRuntime&(std::uint32_t)>;
    using MotionRefresh=std::function<void(std::uint32_t,std::uint32_t)>;
    NativeCombatFleeActions(NativeCombatSkillActorRegistry&,NativeCombatPlayerManager&,
        NativeCombatMonsterManager&,NativeCombatRosterTargets&,NativeCombatDamageCalculator&,
        NativeCombatSystemImage&,Playback,MotionRefresh);
    ~NativeCombatFleeActions();
    NativeCombatFleeActions(const NativeCombatFleeActions&)=delete;
    NativeCombatFleeActions& operator=(const NativeCombatFleeActions&)=delete;
    std::uint32_t Create(std::uint32_t actor);
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
    std::size_t ChildCount() const noexcept;
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
    std::size_t ActiveIterators() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
