#pragma once
#include "pal4/native_combat_task_domain.h"
#include "pal4/native_combat_message_children.h"
#include <functional>
#include <memory>
#include <string_view>
namespace pal4 {
class NativeCombatSkillActorRegistry;
// Per-source PhysicalAttack domain, borrowing real message/AMF services.
// Parent GOM lists use the message owner's allocator. Dependencies outlive it.
// EndAttack construction is retained; its unbound recipe rejects Prepare.
class NativeCombatPhysicalAttackActions final : public INativeCombatTaskDomain {
public:
    using Wounded=std::function<std::uint32_t(std::uint32_t victim,std::uint32_t cause)>;
    using Motion=std::function<void(std::uint32_t actor,std::uint32_t flag)>;
    using EndFactory=std::function<std::uint32_t(std::uint32_t physical_predecessor)>;
    NativeCombatPhysicalAttackActions(std::uint32_t source,NativeCombatSkillActorRegistry&,
        NativeCombatMessageChildren&,NativeCombatMessageChildren::AttackProcessor,
        Wounded,Motion,EndFactory={});
    ~NativeCombatPhysicalAttackActions();
    NativeCombatPhysicalAttackActions(const NativeCombatPhysicalAttackActions&)=delete;
    NativeCombatPhysicalAttackActions& operator=(const NativeCombatPhysicalAttackActions&)=delete;
    // Borrow the real StartAttack predecessor only during this call.
    std::uint32_t CreateSuccessor(std::uint32_t start_attack);
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
