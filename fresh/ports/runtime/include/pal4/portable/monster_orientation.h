#pragma once
#include "pal4/portable/monster_movement_world.h"
#include "pal4/portable/scene_movement.h"
#include "pal4/monster_path_endpoint_update.h"

namespace pal4::portable {
// Original C02/C03 and class3 caches are shared across actors, not owned per instance.
// The caller retains these for the runtime lifetime, including scene reloads.
struct MonsterOrientationCaches {std::array<MonsterPathEndpointUpdateGlobals,2> kinds;MonsterPathEndpointUpdateGlobals random;};
struct MonsterOrientationResult {bool active{},floor_hit{};SceneMonsterTransform transform;};
// Post-grid behavior update: primary floor correction then actor yaw/position.
// Dependencies and shared caches must outlive this service. Single game thread.
class MonsterOrientation final {
public:
    MonsterOrientation(MonsterMovementWorld&,SceneMonsterInstances&,SceneMovement&,MonsterOrientationCaches&);
    ~MonsterOrientation();
    MonsterOrientation(const MonsterOrientation&)=delete;
    MonsterOrientation& operator=(const MonsterOrientation&)=delete;
    // manager_radius is the actual manager +DC field, before its x2 getter.
    // Do not substitute helper radius or a guessed model bound here.
    MonsterOrientationResult Update(std::int32_t kind,MonsterHelperHandle,float manager_radius);
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}
