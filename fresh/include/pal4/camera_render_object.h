#pragma once

#include <cstddef>
#include <cstdint>

namespace pal4 {

struct CameraRenderState final {
    std::uint32_t world = 0u;
    std::uint32_t clump = 0u;
    std::uint32_t source_frame = 0u;
    std::uint32_t dimensions = 0u;
    std::uint32_t target = 0u;
    std::uint32_t active = 0u;
    std::uint32_t camera = 0u;
    std::uint32_t auxiliary_camera = 0u;
    std::uint32_t primary_raster = 0u;
    std::uint32_t stereo_raster = 0u;
    std::uint32_t effects_raster = 0u;
    std::uint32_t current_raster = 0u;
    std::uint32_t first_pipeline = 0u;
    std::uint32_t second_pipeline = 0u;
    std::uint32_t texture = 0u;
    float maximum[3]{};
    float minimum[3]{};
    std::uint32_t frustum_type = 0u;
};

static_assert(sizeof(CameraRenderState) == 88);
static_assert(offsetof(CameraRenderState, active) == 20);
static_assert(offsetof(CameraRenderState, camera) == 24);
static_assert(offsetof(CameraRenderState, current_raster) == 44);
static_assert(offsetof(CameraRenderState, texture) == 56);
static_assert(offsetof(CameraRenderState, maximum) == 60);
static_assert(offsetof(CameraRenderState, minimum) == 72);
static_assert(offsetof(CameraRenderState, frustum_type) == 84);

struct CameraRenderOptions final {
    std::uint32_t raster_shift = 0u;
    bool stereo = false;
};

class ICameraRenderCleanupRuntime {
public:
    virtual ~ICameraRenderCleanupRuntime() = default;
    virtual void DetachTextureRaster(std::uint32_t texture) = 0;
    virtual void DestroyTexture(std::uint32_t texture) = 0;
    virtual void DestroyPipeline(std::uint32_t pipeline) = 0;
    [[nodiscard]] virtual std::uint32_t GetCameraWorld(
        std::uint32_t camera) = 0;
    virtual void RemoveFromWorld(std::uint32_t world,
                                 std::uint32_t camera) = 0;
    virtual void DestroyCamera(std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t DestroyRaster(
        std::uint32_t raster) = 0;
};

class ICameraRenderLifecycleRuntime : public ICameraRenderCleanupRuntime {
public:
    virtual ~ICameraRenderLifecycleRuntime() = default;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t CreateCamera(std::uint32_t mask) = 0;
    virtual void SetFrame(std::uint32_t camera, std::uint32_t frame) = 0;
    virtual void SetClip(std::uint32_t camera, float value) = 0;
    virtual void UpdateTarget(std::uint32_t camera, std::uint32_t target) = 0;
    virtual void AddToWorld(std::uint32_t world, std::uint32_t camera) = 0;
    [[nodiscard]] virtual std::uint32_t CreateRaster(std::uint32_t mask) = 0;
    virtual void SetCameraRaster(std::uint32_t camera,
                                 std::uint32_t raster) = 0;
    [[nodiscard]] virtual std::uint32_t CreatePipeline() = 0;
    [[nodiscard]] virtual std::uint32_t CreateTexture(
        std::uint32_t raster) = 0;
    virtual void WriteByte(std::uint32_t address, std::uint8_t value) = 0;
};

class CameraRenderLifecycleRuntimeStub final
    : public ICameraRenderLifecycleRuntime {
public:
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t CreateCamera(std::uint32_t) override {
        return 0u;
    }
    void SetFrame(std::uint32_t, std::uint32_t) override {}
    void SetClip(std::uint32_t, float) override {}
    void UpdateTarget(std::uint32_t, std::uint32_t) override {}
    void AddToWorld(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CreateRaster(std::uint32_t) override {
        return 0u;
    }
    void SetCameraRaster(std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] std::uint32_t CreatePipeline() override { return 0u; }
    [[nodiscard]] std::uint32_t CreateTexture(std::uint32_t) override {
        return 0u;
    }
    void WriteByte(std::uint32_t, std::uint8_t) override {}
    void DetachTextureRaster(std::uint32_t) override {}
    void DestroyTexture(std::uint32_t) override {}
    void DestroyPipeline(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t GetCameraWorld(std::uint32_t) override {
        return 0u;
    }
    void RemoveFromWorld(std::uint32_t, std::uint32_t) override {}
    void DestroyCamera(std::uint32_t) override {}
    [[nodiscard]] std::uint32_t DestroyRaster(std::uint32_t) override {
        return 0u;
    }
};

class CameraRenderObject final {
public:
    // IDA 0x5fc820.
    [[nodiscard]] static std::uint32_t Initialize(
        CameraRenderState& state, std::uint32_t world,
        std::uint32_t clump, std::uint32_t source_frame,
        std::uint32_t dimensions, std::uint32_t target,
        const CameraRenderOptions& options,
        ICameraRenderLifecycleRuntime& runtime);
    // IDA 0x5fc930.
    [[nodiscard]] static std::uint32_t Cleanup(
        CameraRenderState& state, ICameraRenderCleanupRuntime& runtime);
    // IDA 0x5fcc10.
    [[nodiscard]] static std::uint32_t InitializeTextures(
        CameraRenderState& state, ICameraRenderLifecycleRuntime& runtime);
};

}  // namespace pal4
