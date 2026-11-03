#pragma once

#include "pal4/monster_proximity_checker.h"

namespace pal4 {

class GameMathTables;
class NativeBaseObjectMemory;
class NativePlayerTransformRuntime;
class NativeSceneMovementAsset;

// Borrows the active scene and ActorControl image. Uses the original wall
// clump for visibility and the existing resource setter for return arrival.
class NativeMonsterProximityRuntime final : public IMonsterProximityRuntime {
public:
    NativeMonsterProximityRuntime(
        NativeBaseObjectMemory& memory, std::uint32_t actor_control,
        NativeSceneMovementAsset& movement,
        NativePlayerTransformRuntime& transforms, const GameMathTables& math);

    MonsterProximityActor ControlledActor() override;
    double PlanarDistance(float dx, float dz) override;
    float DetectionRadius(std::uint32_t config_address) override;
    std::uint32_t CollisionClump() override;
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t clump_address) override;
    void NotifyReturnTargetReached(std::uint32_t manager_address,
                                   std::uint32_t saved_position_address,
                                   std::uint32_t argument) override;

private:
    NativeBaseObjectMemory& memory_;
    std::uint32_t actor_control_;
    NativeSceneMovementAsset& movement_;
    NativePlayerTransformRuntime& transforms_;
    const GameMathTables& math_;
};

}  // namespace pal4
