#pragma once

#include "pal4/object_movement_processor.h"

#include <span>

namespace pal4 {

class IObjectComplexCollisionRuntime {
public:
    virtual ~IObjectComplexCollisionRuntime() = default;
    [[nodiscard]] virtual float Radius(std::uint32_t object) = 0;
    [[nodiscard]] virtual float Speed(std::uint32_t object) = 0;
    [[nodiscard]] virtual BehaviorVector3 Position(std::uint32_t object) = 0;
    [[nodiscard]] virtual BehaviorVector3 Offset(std::uint32_t object) = 0;
    [[nodiscard]] virtual BehaviorVector3 Forward(std::uint32_t object) = 0;
    [[nodiscard]] virtual BehaviorVector3 Direction(std::uint32_t object) = 0;
    [[nodiscard]] virtual BehaviorVector3 GlobalZeroVector() = 0;
    virtual void NotifyProximity(std::uint32_t object,
                                 std::uint32_t other,
                                 std::uint32_t argument) = 0;
    virtual void NotifyCollision(
        std::uint32_t object, std::uint32_t other, float sign,
        const BehaviorVector3& projected_self,
        const BehaviorVector3& projected_other) = 0;
};

class ObjectComplexCollisionRuntimeStub final
    : public IObjectComplexCollisionRuntime {
public:
    float Radius(std::uint32_t object) override;
    float Speed(std::uint32_t object) override;
    BehaviorVector3 Position(std::uint32_t object) override;
    BehaviorVector3 Offset(std::uint32_t object) override;
    BehaviorVector3 Forward(std::uint32_t object) override;
    BehaviorVector3 Direction(std::uint32_t object) override;
    BehaviorVector3 GlobalZeroVector() override;
    void NotifyProximity(std::uint32_t object, std::uint32_t other,
                         std::uint32_t argument) override;
    void NotifyCollision(std::uint32_t object, std::uint32_t other,
                         float sign, const BehaviorVector3& projected_self,
                         const BehaviorVector3& projected_other) override;
};

class ObjectComplexCollision final {
public:
    static BehaviorVector3& Process(
        ObjectMovementState& state, std::uint32_t self_address,
        BehaviorVector3& output, float maximum_time,
        std::span<const std::uint32_t> objects,
        IObjectComplexCollisionRuntime& runtime);
};

}  // namespace pal4
