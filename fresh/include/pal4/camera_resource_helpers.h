#pragma once

#include "pal4/camera_render_geometry.h"

#include <cstdint>

namespace pal4 {

class ICameraResourceRuntime {
public:
    virtual ~ICameraResourceRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(
        std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteWord(std::uint32_t address,
                           std::uint32_t value) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCamera() = 0;
    [[nodiscard]] virtual std::uint32_t CreateFrame() = 0;
    virtual void AttachFrame(std::uint32_t camera,
                             std::uint32_t frame) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(
        std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t type) = 0;
    virtual void SetProjection(std::uint32_t camera,
                               std::uint32_t type) = 0;
    virtual void DestroyFrame(std::uint32_t frame) = 0;
    virtual void DestroyRaster(std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyCamera(
        std::uint32_t camera) = 0;
    virtual void SetFarClip(std::uint32_t camera, float value) = 0;
    virtual void SetNearClip(std::uint32_t camera, float value) = 0;
    virtual void SetViewWindow(std::uint32_t camera,
                               float width, float height) = 0;
    virtual void CopyMatrix(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void UpdateFrame(std::uint32_t frame) = 0;
    virtual void TranslateFrame(std::uint32_t frame,
                                const CameraRenderVector3& offset,
                                std::uint32_t mode) = 0;
};

class CameraResourceHelpers final {
public:
    // IDA 0x5fd690, formerly labelled Light_Destroy.
    [[nodiscard]] static std::uint32_t Destroy(
        std::uint32_t camera, ICameraResourceRuntime& runtime);
    // IDA 0x5fd6f0, formerly labelled Light_Create.
    [[nodiscard]] static std::uint32_t Create(
        std::uint32_t raster_size, ICameraResourceRuntime& runtime);
    // IDA 0x5fd750.
    [[nodiscard]] static std::uint32_t SetClipAndWindow(
        std::uint32_t camera, float size,
        ICameraResourceRuntime& runtime);
    // IDA 0x5fd7b0.
    [[nodiscard]] static std::uint32_t CopyFrame(
        std::uint32_t camera, std::uint32_t source,
        ICameraResourceRuntime& runtime);
    // IDA 0x5fd7e0.
    [[nodiscard]] static std::uint32_t UpdateTarget(
        std::uint32_t camera, std::uint32_t target,
        ICameraResourceRuntime& runtime);
};

}  // namespace pal4
