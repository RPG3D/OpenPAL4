#pragma once

#include "pal4/camera_data.h"

#include <array>
#include <cstdint>
#include <span>

namespace pal4 {

struct CameraDebugVertex {
    std::array<float, 3> position{};
    std::uint32_t argb{};
};

class ICameraDebugRenderRuntime {
public:
    virtual ~ICameraDebugRenderRuntime() = default;
    [[nodiscard]] virtual std::uint32_t Device() = 0;
    [[nodiscard]] virtual std::uint32_t GetD3DRenderState(
        std::uint32_t device, std::uint32_t state) = 0;
    [[nodiscard]] virtual std::uint32_t SetD3DRenderState(
        std::uint32_t device, std::uint32_t state,
        std::uint32_t value) = 0;
    virtual void DrawBoundingBox(CameraDataState& camera,
                                 std::uint32_t box_offset,
                                 std::uint32_t color,
                                 std::uint32_t transform_offset) = 0;
    virtual void SetRenderState(std::uint32_t state,
                                std::uint32_t value) = 0;
    [[nodiscard]] virtual bool Transform(
        std::span<const CameraDebugVertex> vertices,
        std::uint32_t flags) = 0;
    virtual void RenderIndexed(std::uint32_t primitive,
                               std::span<const std::uint16_t> indices) = 0;
    virtual void End() = 0;
};

class CameraDebugRenderer final {
public:
    explicit CameraDebugRenderer(ICameraDebugRenderRuntime& runtime)
        : runtime_(runtime) {}

    // 0x5e9600: optional frustum and target-axis debug primitives.
    [[nodiscard]] std::uint32_t Render(CameraDataState& camera);

private:
    void RenderFrustum(CameraDataState& camera);
    [[nodiscard]] std::uint32_t RenderAxes(CameraDataState& camera);
    ICameraDebugRenderRuntime& runtime_;
};

}  // namespace pal4
