#pragma once
#include <cstdint>

namespace pal4 {
class IActorAnimationControlRuntime;
// Borrowed Combat owners required by the original actor turn boundary.
// Death actions and scene replacements must be owned by the actual battle.
class INativeCombatActorTurnServices {
public:
    virtual ~INativeCombatActorTurnServices() = default;
    virtual std::uint32_t CreateDeathAction(std::uint32_t actor) = 0;
    virtual void RegisterActorAction(std::uint32_t action) = 0;
    virtual void ApplyPendingSceneRecord(std::uint32_t actor) = 0;
    // Original WorldManager+328 transparent-atomic queue; outlives callbacks.
    virtual std::uint32_t GeometryQueue() = 0;
    virtual IActorAnimationControlRuntime& AnimationRuntime() = 0;
};
// Player538040 queries the actual Combat mode only on the >=50-turn branch.
class INativeCombatPlayerTurnServices : public INativeCombatActorTurnServices {
public:
    virtual bool CombatInactive() = 0;
};
}
