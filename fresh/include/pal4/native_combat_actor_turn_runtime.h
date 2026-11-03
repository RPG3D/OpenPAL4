#pragma once
#include "pal4/native_combat_actor_turn_services.h"
#include <functional>
namespace pal4 {
class NativeCombatActorModelOwner;
class NativeCombatDeathActions;
class NativeCombatTaskQueue;
class NativeCombatPlayerModelCache;
struct NativeCombatSystemImage;
// Borrow one Monster or Player and the battle's death/world queue providers.
// Player mode comes from actual CombatSystem storage; Monster pending-record
// replacement remains a separate optional binding.
// Bind after constructing death services/actions/queue, before executing turns.
// Retire tasks and restore clump callbacks before these dependencies expire.
class NativeCombatActorTurnRuntime final : public INativeCombatPlayerTurnServices {
public:
    NativeCombatActorTurnRuntime(NativeCombatActorModelOwner&,
        std::function<std::uint32_t()> geometry_queue,
        std::function<void(std::uint32_t)> pending_record = {});
    NativeCombatActorTurnRuntime(NativeCombatPlayerModelCache&,NativeCombatSystemImage&,
        std::function<std::uint32_t()> geometry_queue);
    std::uint32_t ActorAddress() const noexcept { return actor_; }
    bool CombatInactive() override;
    void BindDeathQueue(NativeCombatDeathActions&,NativeCombatTaskQueue&);
    std::uint32_t CreateDeathAction(std::uint32_t actor) override;
    void RegisterActorAction(std::uint32_t action) override;
    void ApplyPendingSceneRecord(std::uint32_t actor) override;
    std::uint32_t GeometryQueue() override;
    IActorAnimationControlRuntime& AnimationRuntime() override;
private:
    void CheckActor(std::uint32_t) const;
    NativeCombatActorModelOwner* model_{};
    NativeCombatPlayerModelCache* player_{};
    NativeCombatSystemImage* combat_{};
    std::uint32_t actor_;
    std::function<std::uint32_t()> geometry_queue_;
    std::function<void(std::uint32_t)> pending_record_;
    NativeCombatDeathActions* actions_{};
    NativeCombatTaskQueue* queue_{};
};
}
