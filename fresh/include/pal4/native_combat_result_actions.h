#pragma once
#include "pal4/native_combat_task_domain.h"
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
namespace pal4 {
class NativeCombatSkillActorRegistry;class NativeCombatReactionChildren;
struct NativeCombatSystemImage;
// Shared per-actor Win(568860/5661E0) and LevelUp(568A80/5664A0) owner. Borrows generation-checked actors,
// mixed GOM children and the actual model's virtual444 formation lookup.
class NativeCombatResultActions final : public INativeCombatTaskDomain {
public:
    using Position=std::function<std::array<float,3>()>;
    using RestoreModel=std::function<void()>;
    NativeCombatResultActions(std::uint32_t,NativeCombatSkillActorRegistry&,NativeCombatSystemImage&,NativeCombatReactionChildren&,Position,RestoreModel);
    ~NativeCombatResultActions();
    std::uint32_t Create();
    std::uint32_t CreateLevelUp();
    bool Owns(std::uint32_t)const noexcept override;
    bool Ready(std::uint32_t)const override;
    bool Enabled(std::uint32_t)const override;
    void SetEnabled(std::uint32_t,bool)override;
    std::uint32_t Identity(std::uint32_t)const override;
    bool Processed(std::uint32_t)const override;
    void Prepare(std::uint32_t)override;
    void BeginWaiting(std::uint32_t)override;
    void Update(std::uint32_t,float)override;
    void Stop(std::uint32_t)override;
    std::uint32_t ResultEvent(std::uint32_t)override;
    void Cancel(std::uint32_t)override;
    void Close();
    std::size_t Count()const noexcept;
    std::size_t OwnedAllocations()const noexcept;
    std::size_t OwnedTexts()const noexcept;
private:
    class Impl;std::unique_ptr<Impl> impl_;
};
}
