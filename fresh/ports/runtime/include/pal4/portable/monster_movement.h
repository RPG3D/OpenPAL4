#pragma once
#include "pal4/portable/monster_movement_world.h"
#include <span>

namespace pal4::portable {
// Original five-word obstacle record. The first word is preserved without
// assigning it a meaning not established by the collision consumer.
struct MonsterMovementObstacle {
    std::uint32_t word0{};
    float radius{};
    BehaviorVector3 center{};
};
struct MonsterMovementStats {
    std::uint64_t frames{},random_draws{},sphere_queries{},neighbor_visits{},
        candidate_tests{},candidate_hits{},proximity{},predicted{},drift_updates{},
        path_corrections{},direction_flips{};
};
struct MonsterMovementResult {bool active{};BehaviorVector3 requested{};};
// Original endpoint and random helper frame composition, independent of actor rendering.
// World must outlive this service. Single game thread, no reentry. Errors can
// leave a partially advanced frame and consumed shared RNG: stop/rebuild it.
class MonsterMovement final {
public:
    explicit MonsterMovement(MonsterMovementWorld&);
    ~MonsterMovement();
    MonsterMovement(const MonsterMovement&)=delete;
    MonsterMovement& operator=(const MonsterMovement&)=delete;
    // Caller must supply its actual ordered obstacle list; no default fallback.
    MonsterMovementResult Update(MonsterHelperHandle,
        std::span<const MonsterMovementObstacle>,float elapsed);
    const MonsterMovementStats& Stats() const;
    bool UsesWorld(const MonsterMovementWorld&) const noexcept;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
