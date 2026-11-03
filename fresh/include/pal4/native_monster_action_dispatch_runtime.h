#pragma once

#include "pal4/monster_action_dispatcher.h"
#include "pal4/monster_object_interaction_callback.h"

namespace pal4 {

class NativeBaseObjectMemory;
class NativeMonsterManagerOwner;
class NativeMonsterProximityRuntime;

// Binds 0x4345E0/0x434600 to the scene's owned monster collection. This
// propagates an existing target; it does not initialize combat.
class NativeMonsterActionDispatchRuntime final :
    public IMonsterActionDispatchRuntime,
    public IMonsterObjectInteractionRuntime {
public:
    NativeMonsterActionDispatchRuntime(
        NativeMonsterManagerOwner& manager, NativeBaseObjectMemory& memory,
        NativeMonsterProximityRuntime& proximity)
        : manager_(manager), memory_(memory), proximity_(proximity) {}
    std::uint32_t ManagerInstance() override;
    std::uint32_t IterateAndCallback(std::uint32_t instance,
                                     std::uint32_t callback,
                                     std::uint32_t context) override;
    MonsterInteractionContext Context(std::uint32_t object) override;
    MonsterInteractionObject Candidate(std::uint32_t object) override;
    double TargetDoubleProperty(std::uint32_t target) override;
    double PlanarDistance(float dx, float dz) override;
    float InteractionRadius(std::uint32_t source) override;
    std::uint32_t CollisionClump() override;
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t clump) override;
    void EnterPursuit(std::uint32_t candidate, std::uint32_t target) override;

private:
    NativeMonsterManagerOwner& manager_;
    NativeBaseObjectMemory& memory_;
    NativeMonsterProximityRuntime& proximity_;
};

}  // namespace pal4
