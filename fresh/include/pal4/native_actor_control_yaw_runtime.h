#pragma once

#include "pal4/pal_actor_control_yaw_input.h"

namespace pal4 {

class InputManager;
class NativeCameraManagerOwner;

// Connects IDA 0x424fc0 to the owned input image and supplied camera setter.
class NativeActorControlYawRuntime final : public IActorControlYawRuntime {
public:
    NativeActorControlYawRuntime(const InputManager& input,
                                 NativeCameraManagerOwner& cameras)
        : input_(input), cameras_(cameras) {}

    ActorControlMouseInput ReadYawInput() override;
    float ReadCameraYaw(std::uint32_t camera_internal) override;
    void ApplyCameraYaw(std::uint32_t camera_internal, float yaw,
                        bool update_matrix) override;

private:
    const InputManager& input_;
    NativeCameraManagerOwner& cameras_;
};

}  // namespace pal4
