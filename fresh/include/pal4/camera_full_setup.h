#pragma once

#include "pal4/camera_data.h"
#include "pal4/camera_system_object.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace pal4 {

class CameraAllocation;

struct CameraSetupParameters {
    const char* name{};
    CameraRect viewport{};
    bool depth_raster{};
    std::array<float, 3> target{};
    float distance{};
    float angle_x{};
    float angle_y{};
    float angle_z{};
};

class ICameraFullSetupRuntime {
public:
    virtual ~ICameraFullSetupRuntime() = default;
    [[nodiscard]] virtual std::uint32_t AllocateData(
        std::uint32_t bytes) = 0;
    virtual void ConstructData(std::uint32_t camera_data) = 0;
    [[nodiscard]] virtual float NearClip() = 0;
    [[nodiscard]] virtual float FarClip() = 0;
    [[nodiscard]] virtual bool InitializeDevice(
        std::uint32_t camera_data, std::int32_t width,
        std::int32_t height, bool depth_raster,
        float near_clip, float far_clip) = 0;
    [[nodiscard]] virtual float ViewScale() = 0;
    [[nodiscard]] virtual std::int32_t ViewMode() = 0;
    virtual void AdjustViewWindow(std::uint32_t camera_data,
                                  CameraRect& rectangle, float scale,
                                  std::int32_t mode) = 0;
    virtual void SetParameters(
        std::uint32_t camera_data,
        const std::array<float, 3>& target,
        float angle_y, float angle_x, float angle_z,
        float distance) = 0;
    virtual void InitializeName(StringBufferState& name,
                                const char* text, std::size_t length) = 0;
};

class CameraFullSetup final {
public:
    CameraFullSetup(CameraAllocation& allocation,
                    ICameraFullSetupRuntime& runtime)
        : allocation_(allocation), runtime_(runtime) {}

    // 0x5eab50: replace camera allocation and apply device, viewport and pose.
    [[nodiscard]] bool Initialize(CameraSystemObjectState& owner,
                                  const CameraSetupParameters& parameters);

private:
    CameraAllocation& allocation_;
    ICameraFullSetupRuntime& runtime_;
};

}  // namespace pal4
