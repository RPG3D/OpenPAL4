#pragma once
#include "pal4/camera_render_geometry.h"

namespace pal4 {
// Live SDK object adapter for the shared shadow camera geometry routines.
class NativeCameraRenderGeometryRuntime final : public ICameraRenderGeometryRuntime {
public:
    std::uint32_t ReadWord(std::uint32_t address) override;
    float ReadFloat(std::uint32_t address) override;
    void WriteFloat(std::uint32_t address, float value) override;
    std::uint32_t GetTransform(std::uint32_t frame) override;
    void CopyMatrix(std::uint32_t destination, std::uint32_t source) override;
    void UpdateFrame(std::uint32_t frame) override;
    void Normalize(CameraRenderVector3& value) override;
    void ApplyTransform(std::uint32_t frame, std::uint32_t matrix, std::uint32_t mode) override;
    std::uint32_t SetClip(std::uint32_t camera, float distance) override;
};
}
