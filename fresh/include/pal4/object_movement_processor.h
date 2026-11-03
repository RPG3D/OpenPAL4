#pragma once

#include "pal4/behavior_path_endpoint_helper.h"

#include <array>
#include <cstdint>
#include <span>

namespace pal4 {

struct ObjectMovementState {
    std::array<std::uint32_t, 50> words{};
};
static_assert(sizeof(ObjectMovementState) == 0xc8u);

struct ObjectCollisionCandidate {
    std::array<std::uint32_t, 9> words{};

    [[nodiscard]] bool HasHit() const { return words[0] != 0u; }
    [[nodiscard]] float Time() const;
    ObjectCollisionCandidate& InitializeBody();
    ObjectCollisionCandidate& CopyFrom(const ObjectCollisionCandidate& source);
};
static_assert(sizeof(ObjectCollisionCandidate) == 36u);

struct ObjectMovementTarget {
    BehaviorVector3 collision_input{};
    BehaviorVector3 unclamped_input{};
};

class IObjectMovementRuntime {
public:
    virtual ~IObjectMovementRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 Position(ObjectMovementState& state) = 0;
    [[nodiscard]] virtual std::int32_t Rand() = 0;
    [[nodiscard]] virtual float CandidateRange(ObjectMovementState& state) = 0;
    virtual void CalculateCandidate(ObjectMovementState& state,
                                    std::uint32_t object,
                                    ObjectCollisionCandidate& candidate) = 0;
    virtual void NotifyCandidate(ObjectMovementState& state,
                                 std::uint32_t candidate_token,
                                 float range) = 0;
    [[nodiscard]] virtual BehaviorVector3 CandidateResponse(
        ObjectMovementState& state, const ObjectCollisionCandidate& candidate,
        float range) = 0;
    [[nodiscard]] virtual BehaviorVector3 ComparisonVector() = 0;
    [[nodiscard]] virtual float MovementRange(ObjectMovementState& state) = 0;
    virtual void CopyGlobalCollection() = 0;
    virtual void NotifyTarget(ObjectMovementState& state, float range,
                              ObjectMovementTarget& target) = 0;
    [[nodiscard]] virtual BehaviorVector3 ComplexCollision(
        ObjectMovementState& state, const BehaviorVector3& position) = 0;
    [[nodiscard]] virtual float RandomClamped(float value, float radius) = 0;
    [[nodiscard]] virtual BehaviorVector3 DirectionDrift(
        ObjectMovementState& state, float first, float second,
        const BehaviorVector3& position) = 0;
    [[nodiscard]] virtual std::uint32_t ResolveCollisionProxy(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual BehaviorVector3 ClampedCollision(
        ObjectMovementState& state, const BehaviorVector3& position,
        std::uint32_t option, std::uint32_t proxy) = 0;
    [[nodiscard]] virtual BehaviorVector3 UnclampedCollision(
        ObjectMovementState& state, const BehaviorVector3& position,
        std::uint32_t proxy) = 0;
};

class ObjectMovementRuntimeStub final : public IObjectMovementRuntime {
public:
    BehaviorVector3 Position(ObjectMovementState& state) override;
    std::int32_t Rand() override;
    float CandidateRange(ObjectMovementState& state) override;
    void CalculateCandidate(ObjectMovementState& state, std::uint32_t object,
                            ObjectCollisionCandidate& candidate) override;
    void NotifyCandidate(ObjectMovementState& state,
                         std::uint32_t candidate_token,
                         float range) override;
    BehaviorVector3 CandidateResponse(
        ObjectMovementState& state, const ObjectCollisionCandidate& candidate,
        float range) override;
    BehaviorVector3 ComparisonVector() override;
    float MovementRange(ObjectMovementState& state) override;
    void CopyGlobalCollection() override;
    void NotifyTarget(ObjectMovementState& state, float range,
                      ObjectMovementTarget& target) override;
    BehaviorVector3 ComplexCollision(ObjectMovementState& state,
                                     const BehaviorVector3& position) override;
    float RandomClamped(float value, float radius) override;
    BehaviorVector3 DirectionDrift(ObjectMovementState& state, float first,
                                   float second,
                                   const BehaviorVector3& position) override;
    std::uint32_t ResolveCollisionProxy(std::uint32_t object) override;
    BehaviorVector3 ClampedCollision(ObjectMovementState& state,
                                     const BehaviorVector3& position,
                                     std::uint32_t option,
                                     std::uint32_t proxy) override;
    BehaviorVector3 UnclampedCollision(ObjectMovementState& state,
                                       const BehaviorVector3& position,
                                       std::uint32_t proxy) override;
};

class ObjectMovementProcessor final {
public:
    static BehaviorVector3& Process(ObjectMovementState& state,
                                    BehaviorVector3& output, float delta,
                                    std::span<const std::uint32_t> candidates,
                                    IObjectMovementRuntime& runtime);
};

}  // namespace pal4
