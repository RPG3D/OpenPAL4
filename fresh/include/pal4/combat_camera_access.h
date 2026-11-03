#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace pal4 {

class ICombatCameraRuntime {
public:
    virtual ~ICombatCameraRuntime() = default;
    [[nodiscard]] virtual std::uint32_t CameraManager() = 0;
    [[nodiscard]] virtual std::uint32_t FindCamera(
        std::uint32_t manager, std::string_view name) = 0;
    [[nodiscard]] virtual std::uint32_t InternalCamera(
        std::uint32_t camera_object) = 0;
    virtual void SetPosition(std::uint32_t camera,
                             std::array<float, 3> position,
                             std::uint32_t update) = 0;
    virtual void SetDistance(std::uint32_t camera, float distance,
                             std::uint32_t update) = 0;
    [[nodiscard]] virtual std::uint32_t SetAngles(
        std::uint32_t camera, std::array<float, 3> angles,
        std::uint32_t update) = 0;
};

class CombatCameraAccess final {
public:
    [[nodiscard]] static std::uint32_t Setup(ICombatCameraRuntime& runtime);
    [[nodiscard]] static std::uint32_t Current(ICombatCameraRuntime& runtime);
};

}  // namespace pal4
