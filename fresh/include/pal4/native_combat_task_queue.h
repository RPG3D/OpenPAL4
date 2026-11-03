#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <functional>
#include <span>
namespace pal4 {
class INativeCombatTaskDomain;
class INativeSkillSystemServices;
// Borrows the action/SkillSystem owners. Registered actions retire through this
// queue; those owners must outlive it. Native std::list owns sequence storage.
class NativeCombatTaskQueue final {
public:
    using IdentityAction=std::function<void(std::uint32_t)>;
    // Actor virtual480 service is required when replacing a processed task.
    // If absent, that path throws instead of claiming the actor was notified.
    // The real singleton's queue exists before any combat actors/domains.
    explicit NativeCombatTaskQueue(INativeSkillSystemServices&,IdentityAction={});
    NativeCombatTaskQueue(INativeCombatTaskDomain&,INativeSkillSystemServices&,IdentityAction={});
    ~NativeCombatTaskQueue();
    NativeCombatTaskQueue(const NativeCombatTaskQueue&)=delete;
    NativeCombatTaskQueue& operator=(const NativeCombatTaskQueue&)=delete;
    [[nodiscard]] std::uint32_t Register(std::uint32_t action);
    void Update(float delta);
    void ResetFlags();
    void Clear();
    //5756D0: ResetFlags then Cleanup on the same queue, under one guard.
    void ResetForStart();
    // One battle queue may borrow distinct actual per-actor action owners.
    // Bind before queueing tasks; every owner must outlive the queue.
    void BindActions(INativeCombatTaskDomain&);
    // Validate/reserve the entire batch before publishing any borrowed owner.
    void BindActions(std::span<INativeCombatTaskDomain* const>);
    // Retire borrowed domain registrations only after every task has drained.
    void DetachActions();
    [[nodiscard]] std::size_t DomainCount() const noexcept;
    [[nodiscard]] std::size_t Count() const noexcept;
    [[nodiscard]] bool IsDispatching() const noexcept;
    // A successor is borrowed between ResultEvent and publication. If its
    // cancellation throws, Clear/ResetForStart must retire it before reuse.
    [[nodiscard]] std::size_t PendingSuccessorCount() const noexcept;
    [[nodiscard]] bool Contains(std::uint32_t action) const noexcept;
    [[nodiscard]] bool UsesActions(const INativeCombatTaskDomain&) const noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
