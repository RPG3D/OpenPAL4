#include "pal4/renderware_pipeline_lifetime.h"

namespace pal4 {

std::uint32_t RenderWarePipelineLifetime::DestroyIfPresent(
    const std::uint32_t pipeline,
    IRenderWarePipelineLifetimeRuntime& runtime) {
    return pipeline == 0u ? 0u : runtime.DestroyPipeline(pipeline);
}

std::uint32_t RenderWarePipelineLifetime::BindAndReleaseResource(
    const std::uint32_t object, const std::uint32_t pipeline,
    IRenderWarePipelineLifetimeRuntime& runtime) {
    runtime.WriteWord(object + 124u, pipeline);
    const auto resource = runtime.ReadWord(object + 52u);
    if (resource != 0u) runtime.FreeResourceEntry(resource);
    return object;
}

}  // namespace pal4
