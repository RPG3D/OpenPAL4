#pragma once
#include "pal4/camera_world_sector_pipeline.h"
#include <rwcore.h>
#include <rpworld.h>

namespace pal4 {
// The original callback address is an identity token. The host supplies the
// corresponding typed game implementation; original EXE addresses are not called.
// The World plugin must be attached before EngineOpen/Start.
class NativeCameraWorldSectorPipelineRuntime final : public ICameraWorldSectorPipelineRuntime {
public:
    explicit NativeCameraWorldSectorPipelineRuntime(RxD3D9AllInOneRenderCallBack render);
    std::uint32_t GetCapabilities() override;
    std::uint32_t ReadWord(std::uint32_t address) override;
    std::uint32_t CreatePipeline() override;
    std::uint32_t LockPipeline(std::uint32_t pipeline) override;
    std::uint32_t GetNodeDefinition() override;
    std::uint32_t AddFragment(std::uint32_t locked, std::uint32_t first,
                              std::uint32_t definition, std::uint32_t last) override;
    void UnlockPipeline(std::uint32_t locked) override;
    std::uint32_t FindNode(std::uint32_t pipeline, std::uint32_t name,
                           std::uint32_t first, std::uint32_t last) override;
    void SetRenderCallback(std::uint32_t node, std::uint32_t callback) override;
    void DestroyPipeline(std::uint32_t pipeline) override;
private:
    RxD3D9AllInOneRenderCallBack render_;
};
}
