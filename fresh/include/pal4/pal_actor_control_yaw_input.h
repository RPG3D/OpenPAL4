#pragma once

#include <cstdint>

namespace pal4 {

struct ActorControlMouseInput {
    std::int32_t primary_count = 0;
    std::uint32_t primary_mode = 0;
    std::uint32_t primary_release = 0;
    std::int32_t secondary_count = 0;
    std::uint32_t secondary_mode = 0;
    std::uint32_t secondary_release = 0;
};

class IActorControlYawRuntime {
public:
    virtual ~IActorControlYawRuntime() = default;
    virtual ActorControlMouseInput ReadYawInput();
    virtual float ReadCameraYaw(std::uint32_t camera_internal);
    virtual void ApplyCameraYaw(std::uint32_t camera_internal,
                                float yaw, bool update_matrix);
};

class PalActorControlYawInput final {
public:
    // IDA 0x424fc0. 0 = idle, 1 = adjusted, 2 = released.
    [[nodiscard]] static std::uint32_t Handle(
        float delta, std::uint32_t camera_internal,
        IActorControlYawRuntime& runtime);
};

}  // namespace pal4
