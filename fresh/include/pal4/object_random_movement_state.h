#pragma once

#include "pal4/object_random_movement_lifecycle.h"

namespace pal4 {

class IObjectRandomMovementSpatialRuntime {
public:
    virtual ~IObjectRandomMovementSpatialRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 Position(
        ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual BehaviorVector3 RandomVector() = 0;
};

class ObjectRandomMovementSpatialRuntimeStub final
    : public IObjectRandomMovementSpatialRuntime {
public:
    BehaviorVector3 Position(ObjectRandomMovementState& state) override;
    BehaviorVector3 RandomVector() override;
};

class ObjectRandomMovementProximity final {
public:
    [[nodiscard]] static bool IsOutsideTargetRadius(
        ObjectRandomMovementState& state,
        IObjectRandomMovementSpatialRuntime& runtime);
};

class ObjectRandomMovementStateUpdate final {
public:
    [[nodiscard]] static std::uint32_t Update(
        ObjectRandomMovementState& state, float delta_time,
        IObjectRandomMovementSpatialRuntime& runtime);
};

}  // namespace pal4
