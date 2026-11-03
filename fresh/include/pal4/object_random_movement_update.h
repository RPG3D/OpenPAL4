#pragma once

#include "pal4/object_random_movement_lifecycle.h"

namespace pal4 {

class IObjectRandomMovementUpdateRuntime {
public:
    virtual ~IObjectRandomMovementUpdateRuntime() = default;
    [[nodiscard]] virtual BehaviorVector3 ProcessCollision(
        ObjectRandomMovementState& state, std::uint32_t input,
        float delta_time) = 0;
    virtual void UpdateBase(ObjectRandomMovementState& state,
                            std::uint32_t input, float delta_time,
                            const BehaviorVector3& collision_result) = 0;
    [[nodiscard]] virtual std::uint32_t MemberVtable(
        std::uint32_t member_address) = 0;
    [[nodiscard]] virtual std::uint32_t PositionToken(
        ObjectRandomMovementState& state) = 0;
    [[nodiscard]] virtual std::uint32_t NotifyMember(
        std::uint32_t captured_vtable, std::uint32_t member_address,
        std::uint32_t position_token) = 0;
};

class ObjectRandomMovementUpdateRuntimeStub final
    : public IObjectRandomMovementUpdateRuntime {
public:
    BehaviorVector3 ProcessCollision(ObjectRandomMovementState& state,
                                     std::uint32_t input,
                                     float delta_time) override;
    void UpdateBase(ObjectRandomMovementState& state, std::uint32_t input,
                    float delta_time,
                    const BehaviorVector3& collision_result) override;
    std::uint32_t MemberVtable(std::uint32_t member_address) override;
    std::uint32_t PositionToken(ObjectRandomMovementState& state) override;
    std::uint32_t NotifyMember(std::uint32_t captured_vtable,
                               std::uint32_t member_address,
                               std::uint32_t position_token) override;
};

class ObjectRandomMovementUpdate final {
public:
    [[nodiscard]] static std::uint32_t Update(
        ObjectRandomMovementState& state, std::uint32_t input,
        float delta_time, std::uint32_t inactive_return,
        IObjectRandomMovementUpdateRuntime& runtime);
};

}  // namespace pal4
