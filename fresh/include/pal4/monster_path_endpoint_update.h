#pragma once

#include "pal4/monster_path_endpoint_restore.h"

#include <cstdint>

namespace pal4 {

struct MonsterPathEndpointUpdateGlobals {
    std::uint8_t guard{};
    float first_radius{};
    float second_radius{};
    BehaviorVector3 position{};
    BehaviorVector3 orientation_one{};
    BehaviorVector3 orientation_two{};
    BehaviorVector3 orientation_three{};
};

class IMonsterPathEndpointUpdateRuntime {
public:
    virtual ~IMonsterPathEndpointUpdateRuntime() = default;
    [[nodiscard]] virtual double ManagerDoubleProperty(
        std::uint32_t manager_address) = 0;
    [[nodiscard]] virtual BehaviorVector3 ReadHelperVector(
        std::uint32_t helper_address, std::uint32_t virtual_slot) = 0;
    virtual void InitializeMovementAdapter() = 0;
    virtual void CheckCollision(BehaviorVector3& position,
                                float first_radius,
                                float second_radius) = 0;
    virtual void ApplyHelperPosition(std::uint32_t helper_address,
                                     const BehaviorVector3& position) = 0;
    virtual void UpdateOrientation(std::uint32_t manager_address,
                                   const BehaviorVector3& orientation_one,
                                   const BehaviorVector3& orientation_two,
                                   const BehaviorVector3& orientation_three,
                                   const BehaviorVector3& position) = 0;
};

class MonsterPathEndpointUpdateRuntimeStub final
    : public IMonsterPathEndpointUpdateRuntime {
public:
    double ManagerDoubleProperty(std::uint32_t manager_address) override;
    BehaviorVector3 ReadHelperVector(std::uint32_t helper_address,
                                     std::uint32_t virtual_slot) override;
    void InitializeMovementAdapter() override;
    void CheckCollision(BehaviorVector3& position, float first_radius,
                        float second_radius) override;
    void ApplyHelperPosition(std::uint32_t helper_address,
                             const BehaviorVector3& position) override;
    void UpdateOrientation(std::uint32_t manager_address,
                           const BehaviorVector3& orientation_one,
                           const BehaviorVector3& orientation_two,
                           const BehaviorVector3& orientation_three,
                           const BehaviorVector3& position) override;
};

class MonsterPathEndpointUpdate final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e4280u;
    static constexpr std::uint32_t kFirstRadiusAddress = 0x8e427cu;
    static constexpr std::uint32_t kSecondRadiusAddress = 0x8e4278u;
    static constexpr std::uint32_t kPositionAddress = 0x8e4284u;
    static constexpr std::uint32_t kOrientationAddress = 0x8a6de8u;

    // IDA 0x435d30: update active path endpoint position and orientation.
    static void Process(const MonsterPathEndpointTargetState& target,
                        const BehaviorPathEndpointState& helper,
                        std::uint32_t helper_address,
                        MonsterPathEndpointUpdateGlobals& globals,
                        IMonsterPathEndpointUpdateRuntime& runtime);
};

class MonsterPathEndpointC03Update final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e4274u;
    static constexpr std::uint32_t kFirstRadiusAddress = 0x8e4270u;
    static constexpr std::uint32_t kSecondRadiusAddress = 0x8e426cu;
    static constexpr std::uint32_t kPositionAddress = 0x8e4290u;
    static constexpr std::uint32_t kOrientationAddress = 0x8a6e0cu;

    // IDA 0x436730: update C03 collision and orientation with separate caches.
    static void Process(const MonsterPathEndpointTargetState& target,
                        const BehaviorPathEndpointState& helper,
                        std::uint32_t helper_address,
                        MonsterPathEndpointUpdateGlobals& globals,
                        IMonsterPathEndpointUpdateRuntime& runtime);
};

class MonsterRandomPointUpdate final {
public:
    static constexpr std::uint32_t kGuardAddress = 0x8e4268u;
    static constexpr std::uint32_t kFirstRadiusAddress = 0x8e4264u;
    static constexpr std::uint32_t kSecondRadiusAddress = 0x8e4260u;
    static constexpr std::uint32_t kPositionAddress = 0x8e429cu;
    static constexpr std::uint32_t kOrientationAddress = 0x8a6e30u;

    // IDA 0x437550: update the ObjectType 3 target's collision/orientation.
    static void Process(const MonsterPathEndpointTargetState& target,
                        const BehaviorPathEndpointState& helper,
                        std::uint32_t helper_address,
                        MonsterPathEndpointUpdateGlobals& globals,
                        IMonsterPathEndpointUpdateRuntime& runtime);
};

}  // namespace pal4
