#pragma once

#include <array>
#include <cstdint>

namespace pal4 {

struct MonsterBehaviorManagerState;

struct MonsterProximityActor {
    std::uint32_t address{};
    bool active{};
    std::array<float, 3> position{};
    std::array<float, 3> collision_position{};
    float half_height{};
};

class IMonsterProximityRuntime {
public:
    virtual ~IMonsterProximityRuntime() = default;
    [[nodiscard]] virtual MonsterProximityActor ControlledActor() = 0;
    [[nodiscard]] virtual double PlanarDistance(float dx, float dz) = 0;
    [[nodiscard]] virtual float DetectionRadius(
        std::uint32_t config_address) = 0;
    [[nodiscard]] virtual std::uint32_t CollisionClump() = 0;
    [[nodiscard]] virtual bool CollisionBlocked(
        const std::array<float, 6>& endpoints,
        std::uint32_t clump_address) = 0;
    virtual void NotifyReturnTargetReached(
        std::uint32_t manager_address,
        std::uint32_t saved_position_address,
        std::uint32_t argument) = 0;
};

class MonsterProximityRuntimeStub final : public IMonsterProximityRuntime {
public:
    MonsterProximityActor ControlledActor() override;
    double PlanarDistance(float dx, float dz) override;
    float DetectionRadius(std::uint32_t config_address) override;
    std::uint32_t CollisionClump() override;
    bool CollisionBlocked(const std::array<float, 6>& endpoints,
                          std::uint32_t clump_address) override;
    void NotifyReturnTargetReached(std::uint32_t manager_address,
                                   std::uint32_t saved_position_address,
                                   std::uint32_t argument) override;
};

class MonsterProximityChecker final {
public:
    // IDA 0x433c50: probe the controlled actor and remember the encounter.
    [[nodiscard]] static bool Check(MonsterBehaviorManagerState& state,
                                    IMonsterProximityRuntime& runtime);
    // IDA 0x433da0: check displacement from the saved encounter position.
    [[nodiscard]] static bool BeyondReturnThreshold(
        const MonsterBehaviorManagerState& state,
        IMonsterProximityRuntime& runtime);
    // IDA 0x433e10: notify the manager when the saved position is reached.
    [[nodiscard]] static bool CheckReturnTargetReached(
        const MonsterBehaviorManagerState& state,
        std::uint32_t manager_address, IMonsterProximityRuntime& runtime);
};

}  // namespace pal4
