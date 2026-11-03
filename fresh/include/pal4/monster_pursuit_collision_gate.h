#pragma once

#include "pal4/monster_pursuit_collision_speed.h"

#include <cstdint>

namespace pal4 {

struct MonsterBehaviorManagerState;

class IMonsterPursuitCollisionGateRuntime {
public:
    virtual ~IMonsterPursuitCollisionGateRuntime() = default;
    [[nodiscard]] virtual bool ProcessPursuit(
        const MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed,
        std::uint32_t target_address) = 0;
    [[nodiscard]] virtual std::uint32_t EnterReturn(
        MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed) = 0;
    [[nodiscard]] virtual std::uint32_t PlayerControlObject() = 0;
    [[nodiscard]] virtual bool PlayerControlFlag(
        std::uint32_t player_address) = 0;
};

class MonsterPursuitCollisionGateRuntimeStub final
    : public IMonsterPursuitCollisionGateRuntime {
public:
    bool ProcessPursuit(const MonsterBehaviorManagerState& state,
                        std::uint32_t manager_address, float elapsed,
                        std::uint32_t target_address) override;
    std::uint32_t EnterReturn(MonsterBehaviorManagerState& state,
                              std::uint32_t manager_address,
                              float elapsed) override;
    std::uint32_t PlayerControlObject() override;
    bool PlayerControlFlag(std::uint32_t player_address) override;

private:
    MonsterPursuitSpeedRuntimeStub pursuit_{};
};

class MonsterPursuitCollisionGate final {
public:
    // IDA 0x435290: route pursuit collision or enter return state.
    [[nodiscard]] static std::uint32_t Process(
        MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, float elapsed,
        IMonsterPursuitCollisionGateRuntime& runtime);
};

}  // namespace pal4
