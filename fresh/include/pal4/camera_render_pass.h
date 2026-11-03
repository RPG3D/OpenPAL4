#pragma once

#include "pal4/camera_render_geometry.h"

#include <cstdint>

namespace pal4 {

struct CameraRenderGlobals final {
    bool render_enabled = false;
    bool special_mode = false;
    bool skip_clump_callbacks = false;
    bool stereo = false;
    std::uint32_t post_process_mode = 0u;
    float fog_strength = 0.0f;
    float frustum_scale = 0.0f;
    std::uint32_t fog_option = 0u;
    std::uint32_t reset_counter = 0u;
};

class ICameraRenderPassRuntime {
public:
    virtual ~ICameraRenderPassRuntime() = default;
    virtual void SetProjection(std::uint32_t texture,
                               std::uint32_t scene, std::uint32_t camera,
                               float strength, float scale) = 0;
    virtual void TraverseSectors(std::uint32_t scene,
                                 std::uint32_t callback,
                                 CameraRenderState& state) = 0;
    virtual void TraceNoop() = 0;
    virtual void RenderCollision(std::uint32_t camera,
                                 std::uint32_t raster,
                                 std::uint32_t world,
                                 const float* bounds,
                                 float strength, float scale,
                                 std::uint32_t fog_option) = 0;
    virtual void UpdateTarget(std::uint32_t camera,
                              std::uint32_t target) = 0;
    virtual void ForAllAtomics(std::uint32_t clump,
                               std::uint32_t callback) = 0;
    virtual void RenderSceneInternal(std::uint32_t camera,
                                     std::uint32_t clump,
                                     bool normal_mode,
                                     std::uint32_t callback,
                                     std::uint32_t callback_argument) = 0;
    virtual void RenderStereo(std::uint32_t stereo_raster,
                              std::uint32_t primary_raster,
                              std::uint32_t auxiliary_camera) = 0;
    virtual void RenderEffects(std::uint32_t raster,
                               std::uint32_t effects_raster,
                               std::uint32_t auxiliary_camera,
                               std::uint32_t mode) = 0;
    [[nodiscard]] virtual float ReadFloat(std::uint32_t address) = 0;
    [[nodiscard]] virtual std::uint32_t RenderFog(
        std::uint32_t auxiliary_camera, std::uint32_t raster,
        float strength, float scale, float radius,
        std::uint32_t option) = 0;
};

class CameraRenderPassRuntimeStub final : public ICameraRenderPassRuntime {
public:
    void SetProjection(std::uint32_t, std::uint32_t, std::uint32_t,
                       float, float) override {}
    void TraverseSectors(std::uint32_t, std::uint32_t,
                         CameraRenderState&) override {}
    void TraceNoop() override {}
    void RenderCollision(std::uint32_t, std::uint32_t, std::uint32_t,
                         const float*, float, float,
                         std::uint32_t) override {}
    void UpdateTarget(std::uint32_t, std::uint32_t) override {}
    void ForAllAtomics(std::uint32_t, std::uint32_t) override {}
    void RenderSceneInternal(std::uint32_t, std::uint32_t, bool,
                             std::uint32_t, std::uint32_t) override {}
    void RenderStereo(std::uint32_t, std::uint32_t,
                      std::uint32_t) override {}
    void RenderEffects(std::uint32_t, std::uint32_t,
                       std::uint32_t, std::uint32_t) override {}
    [[nodiscard]] float ReadFloat(std::uint32_t) override { return 0.0f; }
    [[nodiscard]] std::uint32_t RenderFog(
        std::uint32_t, std::uint32_t, float, float,
        float, std::uint32_t) override { return 0u; }
};

class CameraRenderPass final {
public:
    static constexpr std::uint32_t kVisibilityCallback = 0x5fced0u;
    static constexpr std::uint32_t kPreClumpCallback = 0x5fcf80u;
    static constexpr std::uint32_t kPostClumpCallback = 0x5fd040u;

    // IDA 0x5fca10.
    static void Render(CameraRenderState& state, std::uint32_t scene,
                       CameraRenderGlobals& globals,
                       ICameraRenderGeometryRuntime& geometry,
                       ICameraRenderPassRuntime& runtime);
    // IDA 0x5fcaf0; first native argument after `this` is unused.
    [[nodiscard]] static std::uint32_t RenderScene(
        CameraRenderState& state, std::uint32_t unused,
        std::uint32_t callback, std::uint32_t callback_argument,
        bool move_along_target, const CameraRenderGlobals& globals,
        ICameraRenderGeometryRuntime& geometry,
        ICameraRenderPassRuntime& runtime);
    // IDA 0x5fceb0.
    [[nodiscard]] static std::uint32_t RemoveFromList(
        std::uint32_t candidate, std::uint32_t& slot);
};

}  // namespace pal4
