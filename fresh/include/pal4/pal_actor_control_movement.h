#pragma once

#include "pal4/pal_actor_control.h"

#include <cstdint>

namespace pal4 {

class IActorControlMovementRuntime {
public:
    virtual ~IActorControlMovementRuntime() = default;
    [[nodiscard]] virtual float ComputeTransform(
        PalActorControlState& state, std::uint32_t camera,
        std::uint32_t actor, float& axis_y);
    [[nodiscard]] virtual float CameraYaw(std::uint32_t camera);
    virtual void SetCameraYaw(std::uint32_t camera, float yaw,
                              std::uint32_t keep_look_at);
};

class PalActorControlMovement final {
public:
    // IDA 0x424440. Returns one while an incremental rotation remains.
    [[nodiscard]] static std::uint32_t UpdateYaw(
        PalActorControlState& state, std::uint32_t camera,
        std::uint32_t actor, float step,
        IActorControlMovementRuntime& runtime);
};

}  // namespace pal4
