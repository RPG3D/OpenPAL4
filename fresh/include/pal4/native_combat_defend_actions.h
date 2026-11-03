#pragma once
#include "pal4/native_combat_task_domain.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <span>
namespace pal4 {
class NativeCombatPlayerManager;
class NativeCombatMonsterManager;
class NativeCombatActorSkillRuntime;
class NativeCombatActionChildren;
struct NativeCombatSystemImage;
// Borrows actual actors, GOM children and actor480 motion refresh. Queue and
// Player replay templates must retire before Close and the borrowed owners.
class NativeCombatDefendActions final : public INativeCombatTaskDomain {
public:
    using MotionRefresh=std::function<void(std::uint32_t,std::uint32_t)>;
    NativeCombatDefendActions(NativeCombatPlayerManager&,NativeCombatMonsterManager&,
        std::span<NativeCombatActorSkillRuntime* const>,NativeCombatActionChildren&,
        NativeCombatSystemImage&,MotionRefresh);
    ~NativeCombatDefendActions();
    NativeCombatDefendActions(const NativeCombatDefendActions&)=delete;
    NativeCombatDefendActions& operator=(const NativeCombatDefendActions&)=delete;
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
    std::size_t OwnedAllocations() const noexcept;
    std::size_t OwnedTexts() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
