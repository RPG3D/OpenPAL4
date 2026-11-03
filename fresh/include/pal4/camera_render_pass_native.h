#pragma once
#include "pal4/camera_render_pass.h"
#include "pal4/camera_projection_matrix.h"
#include <rwcore.h>
#include <rpworld.h>
namespace pal4 {
class NativeHookClumpNames;
class NativeCameraRenderPassRuntime final : public ICameraRenderPassRuntime {
public:
    explicit NativeCameraRenderPassRuntime(NativeHookClumpNames& names) : names_(names) {}
    static void RenderSector(RwResEntry*,void*,RwUInt8,RwUInt32) noexcept;
    void SetProjection(std::uint32_t texture,std::uint32_t scene,std::uint32_t camera,float strength,float scale) override;
    void TraverseSectors(std::uint32_t scene,std::uint32_t callback,CameraRenderState& state) override;
    void TraceNoop() override;
    void RenderCollision(std::uint32_t camera,std::uint32_t raster,std::uint32_t world,const float* bounds,
        float strength,float scale,std::uint32_t option) override;
    void UpdateTarget(std::uint32_t camera,std::uint32_t target) override;
    void ForAllAtomics(std::uint32_t clump,std::uint32_t callback) override;
    void RenderSceneInternal(std::uint32_t camera,std::uint32_t clump,bool normal,
        std::uint32_t callback,std::uint32_t argument) override;
    void RenderStereo(std::uint32_t target,std::uint32_t source,std::uint32_t camera) override;
    void RenderEffects(std::uint32_t raster,std::uint32_t effects,std::uint32_t camera,std::uint32_t iterations) override;
    float ReadFloat(std::uint32_t address) override;
    std::uint32_t RenderFog(std::uint32_t camera,std::uint32_t raster,float strength,
        float scale,float radius,std::uint32_t option) override;
private:
    NativeHookClumpNames& names_;
    CameraProjectionGlobals projection_{};
};
}
