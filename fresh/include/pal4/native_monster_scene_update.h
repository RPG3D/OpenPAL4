#pragma once
#include "pal4/native_monster_behavior_session.h"
#include <functional>

namespace pal4 {
class NativeMonsterProximityRuntime;
class NativeArenaCollisionTree;
class IRwDictSchemaRuntime;
class GameMathTables;
class IMonsterInteractionRuntime;
class IMonsterEncounterRuntime;
struct NativeMonsterSceneUpdateServices {
    NativeMonsterManagerOwner& monsters;
    NativeBaseObjectMemory& memory;
    NativeWorldPathPlannerOwner& paths;
    NativeArenaCollisionTree& collision;
    NativeSceneMovementAsset& movement;
    NativePlayerTransformRuntime& transforms;
    IRwDictSchemaRuntime& schema;
    const GameMathTables& math;
    NativeMonsterProximityRuntime& proximity;
    std::uint32_t actor_control;
    IMonsterInteractionRuntime& interaction;
    IMonsterEncounterRuntime& encounter;
    std::function<std::uint32_t()> arena_skills;
    NativeActorBehaviorOrientationCaches& caches;
    std::uint32_t& identifier;
    NativeBehaviorMovement::Random random;
};
// Formal437CF0 composition: grid -> active outer4317F0 -> pursuit/scheduler ->
// position/animation. Stop while all creating scene owners are still alive.
class NativeMonsterSceneUpdate final {
public:
    explicit NativeMonsterSceneUpdate(NativeMonsterSceneUpdateServices);
    ~NativeMonsterSceneUpdate();
    // The original519DC3 forwards the actual RwWorld pointer, not a zero mode.
    void Update(float elapsed,std::uint32_t world);
    void Stop();
    const NativeMonsterBehaviorSessionStats& Stats() const;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
}
