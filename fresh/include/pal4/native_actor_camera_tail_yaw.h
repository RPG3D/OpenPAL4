#pragma once

#include <cstdint>

namespace pal4 {

class BaseObjectState;
class NativeCameraManagerOwner;
class PalActorControlState;

struct NativeActorCameraTailYawResult {
    float correction = 0.0f;
    float signed_component = 0.0f;
    float yaw_after = 0.0f;
    bool computed = false;
    bool updated = false;
};

class NativeActorCameraTailYaw final {
public:
    [[nodiscard]] static std::uint32_t Advance(
        PalActorControlState& control, std::uint32_t camera,
        BaseObjectState& actor, NativeCameraManagerOwner& cameras, float step);
    [[nodiscard]] static NativeActorCameraTailYawResult Flush(
        PalActorControlState& control, BaseObjectState& actor,
        NativeCameraManagerOwner& cameras);
};

}  // namespace pal4
