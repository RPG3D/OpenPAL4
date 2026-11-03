#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <string_view>
#include "pal4/native_combat_fatal_visual_services.h"
#include "pal4/native_combat_task_domain.h"
namespace pal4 {
class INativeCombatDeathSequenceServices;
// Owns52-byte Dead/QuickDead parents, real GOM containers and host names.
// Optional borrowed sequence services enable original QuickDead execution;
// visual services additionally enable the original active Dead recipe.
// The battle supplies its actual mode query and all child services.
class NativeCombatDeathActions final : public INativeCombatTaskDomain {
public:
    explicit NativeCombatDeathActions(std::function<bool()> combat_inactive);
    NativeCombatDeathActions(std::function<bool()> combat_inactive,INativeCombatDeathSequenceServices&);
    NativeCombatDeathActions(std::function<bool()> combat_inactive,INativeCombatDeathSequenceServices&,
                            NativeCombatFatalVisualServices);
    ~NativeCombatDeathActions();
    NativeCombatDeathActions(const NativeCombatDeathActions&)=delete;
    NativeCombatDeathActions& operator=(const NativeCombatDeathActions&)=delete;
    [[nodiscard]] std::uint32_t Create(std::uint32_t source,std::uint32_t target);
    // Original Dead+36 successor. Transfers ownership on success; cancels it
    // with Dead until ResultEvent hands it to the queue. Domain outlives Dead.
    void AttachSuccessor(std::uint32_t dead,std::uint32_t next,INativeCombatTaskDomain&);
    // Borrow a live FendOff/QuickFendOff parent image for this call only.
    // Original568F70/5690E0 query target state and exchange participants;
    // parent vtable determines successor type, without consulting current mode.
    [[nodiscard]] std::uint32_t CreateFendOffSuccessor(std::uint32_t parent,
        const std::function<bool(std::uint32_t actor)>& state_three);
    // Original56B7E0/56B950 use different parent victim fields; type is fixed.
    [[nodiscard]] std::uint32_t CreateWoundedSuccessor(std::uint32_t parent,
        const std::function<bool(std::uint32_t actor)>& state_three);
    // Original56B570 virtual16; its actor544 return is ignored by the queue.
    void CompleteFinalHit(std::uint32_t actor,std::uint32_t cause);
    [[nodiscard]] bool Owns(std::uint32_t action) const noexcept override;
    bool Enabled(std::uint32_t action) const override;
    void SetEnabled(std::uint32_t action,bool enabled) override;
    std::uint32_t Identity(std::uint32_t action) const override;
    bool Processed(std::uint32_t action) const override;
    void Stop(std::uint32_t action) override;
    std::uint32_t ResultEvent(std::uint32_t action) override;
    [[nodiscard]] std::string_view Name(std::uint32_t action) const;
    // Original56B5A0/56B160 followed by5ECEB0 BeginWaiting, as during queue
    // registration. Missing active-mode child services reject before mutation.
    void Start(std::uint32_t action);
    // Individual virtual8 / BeginWaiting boundaries for ScriptTaskQueue.
    void Prepare(std::uint32_t action) override;
    void BeginWaiting(std::uint32_t action) override;
    void Update(std::uint32_t action,float delta) override;
    [[nodiscard]] bool Ready(std::uint32_t action) const override;
    // Retire pending children. Queued actions retire through their queue.
    void Cancel(std::uint32_t action) override;
    void Destroy(std::uint32_t action);
    void Clear();
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] std::size_t OwnedAllocations() const noexcept;
    [[nodiscard]] std::size_t OwnedTexts() const noexcept;
    [[nodiscard]] std::size_t OwnedChildren() const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
