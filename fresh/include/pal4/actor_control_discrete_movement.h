#pragma once

#include "pal4/actor_control_input_component.h"
#include "pal4/actor_control_movement_types.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace pal4 {

struct ActorControlMovementGlobals {
    // Discrete route: 0x8e41bc; vector route: 0x8e419c.
    ActorControlVec3 lower{};
    std::uint32_t lower_tag{};
    ActorControlVec3 upper{};
    std::uint32_t upper_tag{};
};
static_assert(sizeof(ActorControlMovementGlobals) == 32u);

class IActorControlDiscreteMovementRuntime {
public:
    virtual ~IActorControlDiscreteMovementRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CameraSystem() = 0;
    [[nodiscard]] virtual bool CameraActive(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t CameraActiveResult(
        std::uint32_t system);
    virtual void GetActiveCamera(std::uint32_t system) = 0;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t ObjectTransform(
        std::uint32_t object) = 0;
    [[nodiscard]] virtual double ActorProperty(std::uint32_t actor) = 0;
    [[nodiscard]] virtual std::uint32_t Controller() = 0;
    virtual void Normalize(ActorControlVec3& vector) = 0;
    virtual void SetAnimationSpeed(std::uint32_t animation, float speed) = 0;
    [[nodiscard]] virtual std::string AnimationName(
        std::uint32_t animation) = 0;
    virtual void SetAnimationName(std::uint32_t animation,
                                  std::string_view name) = 0;
    virtual void SetAnimationFlag(std::uint32_t animation,
                                  std::uint32_t flag) = 0;
    [[nodiscard]] virtual double AngleFromDot(float value) = 0;
    [[nodiscard]] virtual bool AnyInputActive(
        const ActorControlInputComponentState& state) = 0;
    [[nodiscard]] virtual std::uint32_t MovementAdapter() = 0;
    virtual void PreprocessMovement(std::uint32_t adapter,
                                    ActorControlMovementGlobals& globals,
                                    ActorControlVec3& displacement,
                                    std::uint32_t actor_object,
                                    std::uint32_t first,
                                    std::uint32_t second,
                                    std::uint32_t third) = 0;
    virtual void UpdateMovement(std::uint32_t adapter,
                                ActorControlVec3& position,
                                float lower_radius, float upper_radius,
                                std::uint32_t enabled) = 0;
    [[nodiscard]] virtual bool CanMove(std::uint32_t adapter,
                                       ActorControlVec3& position,
                                       float lower_radius,
                                       float upper_radius) = 0;
    [[nodiscard]] virtual std::uint32_t CanMoveResult(
        std::uint32_t adapter, ActorControlVec3& position,
        float lower_radius, float upper_radius);
    virtual void SetActorPosition(std::uint32_t actor,
                                  const ActorControlVec3& position) = 0;
    virtual void RotateActor(std::uint32_t actor,
                             const ActorControlVec3& rotation,
                             std::uint32_t enabled) = 0;
    virtual void UpdateMinimap(const ActorControlVec3& position,
                               std::uint32_t actor_grid) = 0;
    virtual void UpdateFloorAttributes() = 0;
    [[nodiscard]] virtual std::uint32_t UpdateFloorAttributesResult();
};

class ActorControlDiscreteMovementRuntimeStub
    : public IActorControlDiscreteMovementRuntime {
public:
    std::uint32_t CameraSystem() override;
    bool CameraActive(std::uint32_t system) override;
    void GetActiveCamera(std::uint32_t system) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t ObjectTransform(std::uint32_t object) override;
    double ActorProperty(std::uint32_t actor) override;
    std::uint32_t Controller() override;
    void Normalize(ActorControlVec3& vector) override;
    void SetAnimationSpeed(std::uint32_t animation, float speed) override;
    std::string AnimationName(std::uint32_t animation) override;
    void SetAnimationName(std::uint32_t animation,
                          std::string_view name) override;
    void SetAnimationFlag(std::uint32_t animation, std::uint32_t flag) override;
    double AngleFromDot(float value) override;
    bool AnyInputActive(const ActorControlInputComponentState& state) override;
    std::uint32_t MovementAdapter() override;
    void PreprocessMovement(std::uint32_t adapter,
                            ActorControlMovementGlobals& globals,
                            ActorControlVec3& displacement,
                            std::uint32_t actor_object,
                            std::uint32_t first, std::uint32_t second,
                            std::uint32_t third) override;
    void UpdateMovement(std::uint32_t adapter,
                        ActorControlVec3& position,
                        float lower_radius, float upper_radius,
                        std::uint32_t enabled) override;
    bool CanMove(std::uint32_t adapter, ActorControlVec3& position,
                 float lower_radius, float upper_radius) override;
    void SetActorPosition(std::uint32_t actor,
                          const ActorControlVec3& position) override;
    void RotateActor(std::uint32_t actor,
                     const ActorControlVec3& rotation,
                     std::uint32_t enabled) override;
    void UpdateMinimap(const ActorControlVec3& position,
                       std::uint32_t actor_grid) override;
    void UpdateFloorAttributes() override;
};

}  // namespace pal4
