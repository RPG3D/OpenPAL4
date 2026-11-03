#include "pal4/matfx_world_sector_pipeline.h"

namespace pal4 {

std::uint32_t MatFxWorldSectorPipeline::Create(
    IMatFxWorldSectorPipelineRuntime& runtime) {
    const auto pipeline = runtime.CreatePipeline();
    const auto definition = runtime.NodeDefinitionAddress();
    runtime.SetMipmapping(0u);
    if (pipeline == 0u) return 0u;

    const auto locked = runtime.LockPipeline(pipeline);
    if (locked != 0u &&
        runtime.AddFragment(locked, 0u, definition, 0u) != 0u &&
        runtime.UnlockPipeline(locked) != 0u) {
        const auto node = runtime.FindNode(
            pipeline, runtime.ReadWord(definition), 0u, 0u);
        runtime.SetInstanceCallback(node, kInstanceCallback);
        runtime.SetRenderCallback(node, kRenderCallback);
        return pipeline;
    }

    (void)runtime.DestroyPipeline(pipeline);
    return 0u;
}

}  // namespace pal4
