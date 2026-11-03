#pragma once

#include "pal4/camera_render_object.h"

#include <cstdint>

namespace pal4 {

struct CameraRenderVector3 final {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

static_assert(sizeof(CameraRenderVector3) == 12);

class ICameraRenderGeometryRuntime {
public:
    virtual ~ICameraRenderGeometryRuntime() = default;
    [[nodiscard]] virtual std::uint32_t ReadWord(std::uint32_t address) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    virtual void WriteFloat(std::uint32_t address, float value) = 0;
    [[nodiscard]] virtual std::uint32_t GetTransform(
        std::uint32_t frame) = 0;
    virtual void CopyMatrix(std::uint32_t destination,
                            std::uint32_t source) = 0;
    virtual void UpdateFrame(std::uint32_t frame) = 0;
    virtual void Normalize(CameraRenderVector3& value) = 0;
    virtual void ApplyTransform(std::uint32_t frame,
                                std::uint32_t matrix,
                                std::uint32_t mode) = 0;
    [[nodiscard]] virtual std::uint32_t SetClip(
        std::uint32_t camera, float distance) = 0;
};

class CameraRenderGeometryRuntimeStub final
    : public ICameraRenderGeometryRuntime {
public:
    [[nodiscard]] std::uint32_t ReadWord(std::uint32_t) override { return 0u; }
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    void WriteFloat(std::uint32_t, float) override {}
    [[nodiscard]] std::uint32_t GetTransform(
        std::uint32_t) override { return 0u; }
    void CopyMatrix(std::uint32_t, std::uint32_t) override {}
    void UpdateFrame(std::uint32_t) override {}
    void Normalize(CameraRenderVector3&) override {}
    void ApplyTransform(std::uint32_t, std::uint32_t,
                        std::uint32_t) override {}
    [[nodiscard]] std::uint32_t SetClip(std::uint32_t, float) override {
        return 0u;
    }
};

class CameraRenderGeometry final {
public:
    // IDA 0x5fcc50.
    [[nodiscard]] static std::uint32_t UpdateFrustum(
        CameraRenderState& state, bool special_mode, float scale,
        ICameraRenderGeometryRuntime& runtime);
    // IDA 0x5fcd80.
    [[nodiscard]] static std::uint32_t UpdatePosition(
        CameraRenderState& state, bool move_along_target,
        ICameraRenderGeometryRuntime& runtime);
};

}  // namespace pal4
