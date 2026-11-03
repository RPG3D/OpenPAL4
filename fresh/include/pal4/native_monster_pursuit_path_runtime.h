#pragma once

#include "pal4/monster_pursuit_collision_speed.h"
#include "pal4/monster_pursuit_collision_gate.h"

namespace pal4 {

class NativeArenaCollisionTree;
class NativeBaseObjectMemory;
class NativeMonsterManagerOwner;

// Arena path boundary shared by the reviewed chase and return functions.
// actor_memory must own the monsters' CollisionActor path allocations.
class NativeMonsterPursuitPathRuntime final : public IMonsterPursuitReturnRuntime,
                                             public IMonsterPursuitCollisionGateRuntime {
public:
    NativeMonsterPursuitPathRuntime(NativeMonsterManagerOwner& manager,
                                   NativeArenaCollisionTree& tree,
                                   NativeBaseObjectMemory& actor_memory,
                                   std::uint32_t player_control = 0u);

    bool ProcessPursuit(const MonsterBehaviorManagerState& state,
                        std::uint32_t object, float elapsed,
                        std::uint32_t target) override;
    std::uint32_t EnterReturn(MonsterBehaviorManagerState& state,
                              std::uint32_t object, float elapsed) override;
    std::uint32_t PlayerControlObject() override;
    bool PlayerControlFlag(std::uint32_t control) override;

    MonsterPursuitTarget Target(std::uint32_t target) override;
    std::uint32_t FindPursuitPath(float elapsed,
                                 std::array<float, 3>& point) override;
    bool ApplyPursuitPath(std::uint32_t movement,
                          const std::array<float, 3>& from,
                          const std::array<float, 3>& to,
                          std::uint32_t face, std::uint32_t argument) override;
    float MaxPursuitSpeed(std::uint32_t source, float fallback) override;
    std::uint32_t SetPursuitSpeed(std::uint32_t movement, float speed) override;
    std::uint32_t ResetPursuit(MonsterBehaviorManagerState& state,
                              std::uint32_t object) override;

private:
    void RequireMovement(std::uint32_t movement, bool allow_unbound = false) const;
    NativeMonsterManagerOwner& manager_;
    NativeArenaCollisionTree& tree_;
    NativeBaseObjectMemory& memory_;
    std::uint32_t player_control_;
};

}  // namespace pal4
