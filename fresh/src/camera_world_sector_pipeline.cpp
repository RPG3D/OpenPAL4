#include "pal4/camera_world_sector_pipeline.h"

namespace pal4 {

std::uint32_t CameraWorldSectorPipeline::Create(
    ICameraWorldSectorPipelineRuntime& runtime) {
    const auto capabilities = runtime.GetCapabilities();
    if (runtime.ReadWord(capabilities + 152u) <= 1u) return 0u;
    const auto pipeline = runtime.CreatePipeline();
    if (pipeline == 0u) return 0u;
    const auto locked = runtime.LockPipeline(pipeline);
    if (locked == 0u) {
        runtime.DestroyPipeline(pipeline);
        return 0u;
    }
    const auto definition = runtime.GetNodeDefinition();
    const auto fragment = runtime.AddFragment(locked, 0u, definition, 0u);
    runtime.UnlockPipeline(fragment);
    const auto name = runtime.ReadWord(definition);
    const auto node = runtime.FindNode(pipeline, name, 0u, 0u);
    runtime.SetRenderCallback(node, kRenderCallback);
    return pipeline;
}

}  // namespace pal4
