#pragma once

#include "pal4/object_movement_processor.h"

namespace pal4 {

class IObjectCollisionTimeRuntime {
public:
    virtual ~IObjectCollisionTimeRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 RelativeVector(
        ObjectMovementState& state, std::uint32_t candidate_vector) = 0;
    [[nodiscard]] virtual float ObjectRadius(ObjectMovementState& state) = 0;
    [[nodiscard]] virtual float CandidateRadius(
        std::uint32_t candidate_address) = 0;
};

class ObjectCollisionTimeRuntimeStub final : public IObjectCollisionTimeRuntime {
public:
    BehaviorVector3 RelativeVector(
        ObjectMovementState& state,
        std::uint32_t candidate_vector) override;
    float ObjectRadius(ObjectMovementState& state) override;
    float CandidateRadius(std::uint32_t candidate_address) override;
};

class ObjectCollisionTime final {
public:
    static void Calculate(ObjectMovementState& state,
                          std::uint32_t candidate_address,
                          ObjectCollisionCandidate& result,
                          IObjectCollisionTimeRuntime& runtime);
};

}  // namespace pal4
