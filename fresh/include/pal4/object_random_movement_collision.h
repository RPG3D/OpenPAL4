#pragma once

#include "pal4/object_movement_processor.h"
#include "pal4/object_random_movement_state.h"

#include <cstdint>
#include <span>

namespace pal4 {

// The obstacle response is shared with path movement and needs only these
// four getters. Keep it independent of random-walk selection and scanning.
class IObjectObstacleCorrectionRuntime {
public:
    virtual ~IObjectObstacleCorrectionRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 CollisionPosition(ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual BehaviorVector3 CandidateVector(std::uint32_t address) = 0;
    [[nodiscard]] virtual BehaviorVector3 CollisionAxis(ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual float CorrectionScale(ObjectRandomMovementState& state) = 0;
};
class IObjectRandomMovementCollisionRuntime
    : public IObjectRandomMovementSpatialRuntime, public IObjectObstacleCorrectionRuntime {
public:
    [[nodiscard]] virtual BehaviorVector3 BaseVector(
        ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual std::int32_t Rand() = 0;
    [[nodiscard]] virtual float Speed(ObjectRandomMovementState& state) = 0;
    virtual void CalculateCandidate(ObjectRandomMovementState& state,
                                    std::uint32_t candidate_address,
                                    ObjectCollisionCandidate& record) = 0;
    virtual void NotifyCandidate(ObjectRandomMovementState& state,
                                 std::uint32_t candidate_address,
                                 float range) = 0;
    [[nodiscard]] virtual BehaviorVector3 CollisionPosition(
        ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual BehaviorVector3 CandidateVector(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual BehaviorVector3 CollisionAxis(
        ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual float CorrectionScale(
        ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual BehaviorVector3 GlobalZeroVector() = 0;
    [[nodiscard]] virtual float FallbackRange(
        ObjectRandomMovementState& state) = 0;
    virtual void CopyGlobalCollection() = 0;
    [[nodiscard]] virtual BehaviorVector3 NotifyMemberScan(
        ObjectRandomMovementState& state, std::uint32_t member_address,
        float range) = 0;
    [[nodiscard]] virtual BehaviorVector3 ComplexCollision(
        ObjectRandomMovementState& state,
        const BehaviorVector3& position, float range) = 0;
};

class ObjectRandomMovementCollisionRuntimeStub final
    : public IObjectRandomMovementCollisionRuntime {
public:
    BehaviorVector3 Position(ObjectRandomMovementState& state) override;
    BehaviorVector3 RandomVector() override;
    BehaviorVector3 BaseVector(ObjectRandomMovementState& state) override;
    std::int32_t Rand() override;
    float Speed(ObjectRandomMovementState& state) override;
    void CalculateCandidate(ObjectRandomMovementState& state,
                            std::uint32_t candidate_address,
                            ObjectCollisionCandidate& record) override;
    void NotifyCandidate(ObjectRandomMovementState& state,
                         std::uint32_t candidate_address,
                         float range) override;
    BehaviorVector3 CollisionPosition(
        ObjectRandomMovementState& state) override;
    BehaviorVector3 CandidateVector(std::uint32_t address) override;
    BehaviorVector3 CollisionAxis(ObjectRandomMovementState& state) override;
    float CorrectionScale(ObjectRandomMovementState& state) override;
    BehaviorVector3 GlobalZeroVector() override;
    float FallbackRange(ObjectRandomMovementState& state) override;
    void CopyGlobalCollection() override;
    BehaviorVector3 NotifyMemberScan(ObjectRandomMovementState& state,
                                     std::uint32_t member_address,
                                     float range) override;
    BehaviorVector3 ComplexCollision(ObjectRandomMovementState& state,
                                     const BehaviorVector3& position,
                                     float range) override;
};

class ObjectRandomMovementCollision final {
public:
    // Shared arithmetic also present in42AB30's obstacle-response branch.
    [[nodiscard]] static BehaviorVector3 ObstacleCorrection(
        ObjectRandomMovementState& state,const ObjectCollisionCandidate& candidate,
        IObjectObstacleCorrectionRuntime& runtime);
    static BehaviorVector3& Process(
        ObjectRandomMovementState& state, BehaviorVector3& output,
        float delta_time, std::span<const std::uint32_t> candidates,
        IObjectRandomMovementCollisionRuntime& runtime);
};

}  // namespace pal4
