#pragma once

#include "pal4/object_movement_processor.h"

#include <cstdint>

namespace pal4 {

class IObjectPathMovementRuntime {
public:
    virtual ~IObjectPathMovementRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 Position(
        ObjectMovementState& state) = 0;
    [[nodiscard]] virtual float DistanceAlong(
        std::uint32_t path_address, const BehaviorVector3& position) = 0;
    [[nodiscard]] virtual float Speed(ObjectMovementState& state) = 0;
    [[nodiscard]] virtual float PathLength(std::uint32_t path_address) = 0;
    [[nodiscard]] virtual BehaviorVector3 PointAt(
        std::uint32_t path_address, float distance) = 0;
    [[nodiscard]] virtual BehaviorVector3 LastPoint(
        std::uint32_t path_address) = 0;
    virtual void SetPosition(ObjectMovementState& state,
                             const BehaviorVector3& position) = 0;
    [[nodiscard]] virtual std::uint32_t NotifyMember(
        std::uint32_t member_address,
        const BehaviorVector3& path_point) = 0;
};

class ObjectPathMovementRuntimeStub final : public IObjectPathMovementRuntime {
public:
    BehaviorVector3 Position(ObjectMovementState& state) override;
    float DistanceAlong(std::uint32_t path_address,
                        const BehaviorVector3& position) override;
    float Speed(ObjectMovementState& state) override;
    float PathLength(std::uint32_t path_address) override;
    BehaviorVector3 PointAt(std::uint32_t path_address,
                            float distance) override;
    BehaviorVector3 LastPoint(std::uint32_t path_address) override;
    void SetPosition(ObjectMovementState& state,
                     const BehaviorVector3& position) override;
    std::uint32_t NotifyMember(std::uint32_t member_address,
                                const BehaviorVector3& path_point) override;
};

class ObjectPathMovement final {
public:
    [[nodiscard]] static std::uint32_t Advance(
        ObjectMovementState& state, std::uint32_t unused, float delta_time,
        IObjectPathMovementRuntime& runtime);
};

}  // namespace pal4
