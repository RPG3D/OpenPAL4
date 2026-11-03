#pragma once

#include "pal4/camera_data.h"
#include "pal4/camera_bounds.h"
#include "pal4/camera_matrix.h"
#include "pal4/native_camera_manager_owner.h"
#include "pal4/native_renderware_camera_task_geometry.h"

namespace pal4 {

// RenderWare SDK boundary for the four startup camera objects.
class NativeCameraObjectResources final : public INativeCameraObjectResources,
                                          private ICameraDataRuntime,
                                          private ICameraDeviceRuntime,
                                          private ICameraMatrixRuntime,
                                          private ICameraBoundsRuntime {
public:
    NativeCameraObjectResources(CameraRenderRect rect, float near_clip,
                                float far_clip, float view_scale)
        : rect_(rect), near_clip_(near_clip), far_clip_(far_clip),
          view_scale_(view_scale) {}

    [[nodiscard]] CameraRenderRect RenderRect() override { return rect_; }
    void LogInitialization(bool complete) override;
    [[nodiscard]] bool Initialize(
        CameraSystemObjectState& object, CameraDataState& data,
        const CameraSetupParameters& parameters) override;
    void Destroy(CameraSystemObjectState& object,
                 CameraDataState& data) noexcept override;
    [[nodiscard]] float MinimumDistance() override { return near_clip_; }
    [[nodiscard]] float MaximumDistance() override { return far_clip_; }
    [[nodiscard]] std::uint32_t UpdateMatrix(
        CameraDataState& data, std::uint32_t solve_eye) override;
    [[nodiscard]] std::uint32_t AdvanceFrame(
        CameraDataState& data, const CameraControllerState& manager,
        float elapsed) override;
    [[nodiscard]] ICameraTaskGeometry* TaskGeometry() override {
        return &task_geometry_;
    }

private:
    class FrameRuntime;
    [[nodiscard]] bool IntersectsCachedObstacle(CameraDataState& data);
    void InitializeNested(CameraDataState& state,
                          std::uint32_t offset) override;
    [[nodiscard]] std::uint32_t NullFunctionSentinel() override;
    void DestroyCamera(CameraDataState& state) override;
    void DestroyNested(CameraDataState& state,
                       std::uint32_t offset) override;

    [[nodiscard]] std::uint32_t CreateCamera() override;
    [[nodiscard]] std::uint32_t CreateFrame() override;
    void AttachFrame(std::uint32_t camera, std::uint32_t frame) override;
    [[nodiscard]] std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t flags) override;
    void StoreRaster(std::uint32_t camera, std::uint32_t offset,
                     std::uint32_t raster) override;
    void SetNearClip(std::uint32_t camera, float distance) override;
    void SetFarClip(std::uint32_t camera, float distance) override;
    [[nodiscard]] std::uint32_t CameraFrame(std::uint32_t camera) override;
    [[nodiscard]] std::uint32_t CameraRaster(
        std::uint32_t camera, std::uint32_t offset) override;
    void TranslateFrame(std::uint32_t frame,
                        const std::array<float, 3>& translation,
                        std::uint32_t mode) override;
    [[nodiscard]] std::uint32_t CameraWorld(std::uint32_t camera) override;
    void RemoveCameraFromWorld(std::uint32_t world,
                               std::uint32_t camera) override;
    void DestroyFrame(std::uint32_t frame) override;
    void DestroyRaster(std::uint32_t raster) override;
    [[nodiscard]] std::uint32_t DestroyCamera(
        std::uint32_t camera) override;

    void Rotate(CameraDataState& data, std::uint32_t matrix_offset,
                const std::array<float, 3>& axis, float degrees,
                std::uint32_t mode) override;
    void SetFrameIdentity(std::uint32_t frame) override;
    void ApplyTransform(std::uint32_t frame, CameraDataState& data,
                        std::uint32_t matrix_offset,
                        std::uint32_t mode) override;
    void UpdateFrameObjects(std::uint32_t frame) override;
    [[nodiscard]] std::uint32_t CalculateBoundingBox(
        CameraDataState& data, std::uint32_t bounds_offset,
        const CameraBoundsVertices& vertices,
        const CameraBoundsMatrix& transform,
        std::uint32_t camera) override;

    CameraRenderRect rect_{};
    float near_clip_{};
    float far_clip_{};
    float view_scale_{};
    NativeRenderWareCameraTaskGeometry task_geometry_;
};

}  // namespace pal4
