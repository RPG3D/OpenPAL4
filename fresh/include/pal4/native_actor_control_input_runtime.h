#pragma once

#include "pal4/actor_control_discrete_movement.h"
#include "pal4/actor_control_input_events.h"
#include "pal4/actor_control_free_movement.h"
#include "pal4/actor_control_mouse_projection.h"

#include <cstdint>
#include <optional>

namespace pal4 {

class InputManager;
class NativeCameraManagerOwner;

// Binds the ActorControl input component to the owned input image and window.
class NativeActorControlInputRuntime final
    : private IActorControlInputComponentRuntime,
      private IActorControlInputEventRuntime,
      private IActorControlFreeMovementRuntime,
      private IActorControlMouseProjectionRuntime {
public:
    NativeActorControlInputRuntime(
        pal4::InputManager& input, std::uint32_t window,
        IActorControlDiscreteMovementRuntime* movement = nullptr,
        NativeCameraManagerOwner* cameras = nullptr)
        : input_(input), window_(window), movement_(movement),
          cameras_(cameras) {}

    [[nodiscard]] bool Process(std::uint32_t* component,
                               std::uint32_t actor, float elapsed);
    void SetCursorOverride(std::optional<ActorControlCursorPoint> cursor) {
        cursor_override_ = cursor;
    }

private:
    std::int32_t GameConfigPrimary() override;
    void SetAnimationFlag(std::uint32_t animation,
                          std::uint32_t value) override;
    void HandleInputEvents(ActorControlInputComponentState& state,
                           std::uint32_t actor, float elapsed) override;
    void UpdateInputObject(ActorControlInputComponentState& state,
                           std::uint32_t actor, float elapsed) override;
    std::uint32_t InputManager() override;
    std::uint32_t ReadInputWord(std::uint32_t manager,
                                std::uint32_t offset) override;
    ActorControlScreenPoint CursorPoint() override;
    ActorControlScreenRect WindowRect() override;
    void GetInputStateValues(std::uint32_t manager,
                             std::uint32_t& first,
                             std::uint32_t& second) override;
    void DispatchMovement(ActorControlInputComponentState& state,
                          std::uint32_t actor, std::uint32_t command,
                          float elapsed) override;
    void TurnFromMouse(ActorControlInputComponentState& state,
                       std::uint32_t actor,
                       std::int32_t input_value) override;
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
                                float elapsed) override;
    ActorControlCursorPoint CursorPosition() override;
    void ScreenToClient(ActorControlCursorPoint& point) override;
    std::uint32_t ActiveCameraInternalId(std::uint32_t system) override;
    void CalculateNormalizedPixelRay(std::uint32_t camera, float x, float y,
                                     ActorControlRay& ray) override;
    std::uint32_t ReadActorWord(std::uint32_t address) override;
    double CalculateIntersection(const ActorControlRay& ray,
                                 const ActorControlVec3& plane_normal,
                                 const ActorControlVec3& actor_position) override;

    pal4::InputManager& input_;
    std::uint32_t window_{};
    IActorControlDiscreteMovementRuntime* movement_{};
    NativeCameraManagerOwner* cameras_{};
    ActorControlMovementGlobals globals_{};
    std::optional<ActorControlCursorPoint> cursor_override_;
};

}  // namespace pal4
