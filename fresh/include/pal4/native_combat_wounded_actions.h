#pragma once
#include "pal4/native_combat_task_domain.h"
#include <memory>
#include <string_view>
#include <cstddef>
namespace pal4 {
class NativeCombatSkillActorRegistry;
class NativeCombatFatalChildren;
class NativeCombatDeathActions;
class NativeCombatEndSoundEvents;
struct NativeCombatSystemImage;
// Per-target Wounded/QuickWounded domain. Borrows registry/mode and dedicated reaction
// children bound to this target's real playback/396. All outlive this owner.
// FinalHit preparation requires explicitly bound visual/message services and
// completion borrows the matching actor death domain.
class NativeCombatWoundedActions final : public INativeCombatTaskDomain {
public:
    NativeCombatWoundedActions(std::uint32_t target,NativeCombatSkillActorRegistry&,
        NativeCombatSystemImage&,NativeCombatFatalChildren&,NativeCombatDeathActions&,NativeCombatEndSoundEvents&);
    ~NativeCombatWoundedActions();
    NativeCombatWoundedActions(const NativeCombatWoundedActions&)=delete;
    NativeCombatWoundedActions& operator=(const NativeCombatWoundedActions&)=delete;
    std::uint32_t Create(std::uint32_t victim,std::uint32_t cause);
    std::string_view Name(std::uint32_t) const;
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
