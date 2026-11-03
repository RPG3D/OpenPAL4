#pragma once
#include "pal4/matfx_world_sector_pipeline.h"
#include "pal4/camera_world_sector_pipeline_native.h"

namespace pal4 {
// 53E4C0 game factory adapter. Original addresses are identity tokens only.
// Callers supply the game implementations; the factory does not own them.
class NativeMatFxWorldSectorPipelineRuntime final : public IMatFxWorldSectorPipelineRuntime {
public:
    NativeMatFxWorldSectorPipelineRuntime(RxD3D9AllInOneInstanceCallBack instance,
                                        RxD3D9AllInOneRenderCallBack render);
    std::uint32_t CreatePipeline() override;
    std::uint32_t NodeDefinitionAddress() override;
    void SetMipmapping(std::uint32_t enabled) override;
    std::uint32_t LockPipeline(std::uint32_t pipeline) override;
    std::uint32_t AddFragment(std::uint32_t locked, std::uint32_t first,
                             std::uint32_t definition, std::uint32_t last) override;
    std::uint32_t UnlockPipeline(std::uint32_t locked) override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t FindNode(std::uint32_t pipeline, std::uint32_t name,
                         std::uint32_t first, std::uint32_t last) override;
    void SetInstanceCallback(std::uint32_t node, std::uint32_t callback) override;
    void SetRenderCallback(std::uint32_t node, std::uint32_t callback) override;
    std::uint32_t DestroyPipeline(std::uint32_t pipeline) override;
private:
    NativeCameraWorldSectorPipelineRuntime sdk_;
    RxD3D9AllInOneInstanceCallBack instance_;
    RxD3D9AllInOneRenderCallBack render_;
};
}
