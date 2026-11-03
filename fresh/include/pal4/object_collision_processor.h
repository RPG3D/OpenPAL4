#pragma once

#include "pal4/object_movement_processor.h"

namespace pal4 {

struct ObjectCollisionSweep {
    BehaviorVector3 position{};
    float time = -1.0f;
};

class IObjectCollisionRuntime {
public:
    virtual ~IObjectCollisionRuntime() = default;
    [[nodiscard]] virtual float MovementScale(ObjectMovementState& state) = 0;
    [[nodiscard]] virtual BehaviorVector3 Motion(
        ObjectMovementState& state, float delta) = 0;
    [[nodiscard]] virtual BehaviorVector3 Position(
        ObjectMovementState& state) = 0;
    [[nodiscard]] virtual float Measure(
        std::uint32_t target, const BehaviorVector3& vector) = 0;
    [[nodiscard]] virtual ObjectCollisionSweep Sweep(
        std::uint32_t target, const BehaviorVector3& motion) = 0;
    [[nodiscard]] virtual BehaviorVector3 Predict(
        std::uint32_t target, const BehaviorVector3& position,
        float measure) = 0;
    virtual void Adjust(ObjectMovementState& state,
                        const BehaviorVector3& motion,
                        BehaviorVector3& hit_position,
                        BehaviorVector3& predicted,
                        float hit_time) = 0;
    [[nodiscard]] virtual BehaviorVector3 Offset(ObjectMovementState& state) = 0;
    [[nodiscard]] virtual BehaviorVector3 GlobalZeroVector() = 0;
};

class ObjectCollisionRuntimeStub final : public IObjectCollisionRuntime {
public:
    float MovementScale(ObjectMovementState& state) override;
    BehaviorVector3 Motion(ObjectMovementState& state, float delta) override;
    BehaviorVector3 Position(ObjectMovementState& state) override;
    float Measure(std::uint32_t target,
                  const BehaviorVector3& vector) override;
    ObjectCollisionSweep Sweep(
        std::uint32_t target, const BehaviorVector3& motion) override;
    BehaviorVector3 Predict(
        std::uint32_t target, const BehaviorVector3& position,
        float measure) override;
    void Adjust(ObjectMovementState& state,
                const BehaviorVector3& motion,
                BehaviorVector3& hit_position,
                BehaviorVector3& predicted,
                float hit_time) override;
    BehaviorVector3 Offset(ObjectMovementState& state) override;
    BehaviorVector3 GlobalZeroVector() override;
};

class ObjectCollisionProcessor final {
public:
    static BehaviorVector3& ProcessClamped(
        ObjectMovementState& state, BehaviorVector3& output,
        std::int32_t option, float delta, std::uint32_t target,
        IObjectCollisionRuntime& runtime);
    static BehaviorVector3& ProcessUnclamped(
        ObjectMovementState& state, BehaviorVector3& output,
        float delta, std::uint32_t target,
        IObjectCollisionRuntime& runtime);
};

}  // namespace pal4
