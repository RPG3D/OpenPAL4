#pragma once

#include "pal4/actor_control_discrete_movement.h"

#include <cstdint>

namespace pal4 {

class IActorControlFreeMovementRuntime {
public:
    virtual ~IActorControlFreeMovementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t InputManager() = 0;
    [[nodiscard]] virtual std::uint32_t ReadInputWord(
        std::uint32_t manager, std::uint32_t offset) = 0;
    [[nodiscard]] virtual std::uint32_t CameraSystem() = 0;
    [[nodiscard]] virtual std::uint32_t ActiveCameraObjectPointer(
        std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual bool MouseWorldCoordinates(
        ActorControlInputComponentState& state, std::uint32_t actor,
        ActorControlVec3& output) = 0;
    [[nodiscard]] virtual float ControllerThreshold() = 0;
    [[nodiscard]] virtual double ApproximateMagnitude(
        const ActorControlVec3& vector) = 0;
    virtual void NormalizeDirection(
        ActorControlInputComponentState& state) = 0;
    virtual void DispatchVectorMovement(
        ActorControlInputComponentState& state, std::uint32_t actor,
        float delta_seconds) = 0;
};

class ActorControlFreeMovementRuntimeStub final
    : public IActorControlFreeMovementRuntime {
public:
    std::uint32_t InputManager() override;
    std::uint32_t ReadInputWord(std::uint32_t manager,
                                std::uint32_t offset) override;
    std::uint32_t CameraSystem() override;
    std::uint32_t ActiveCameraObjectPointer(std::uint32_t system) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ObjectTransform(std::uint32_t object) override;
    bool MouseWorldCoordinates(ActorControlInputComponentState& state,
                               std::uint32_t actor,
                               ActorControlVec3& output) override;
    float ControllerThreshold() override;
    double ApproximateMagnitude(const ActorControlVec3& vector) override;
    void NormalizeDirection(ActorControlInputComponentState& state) override;
    void DispatchVectorMovement(ActorControlInputComponentState& state,
                                std::uint32_t actor,
                                float delta_seconds) override;
};

}  // namespace pal4
