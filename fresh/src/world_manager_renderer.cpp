#include "pal4/world_manager_renderer.h"

namespace pal4 {

std::uint32_t WorldManagerRenderer::Render(
    const std::uint32_t manager, const std::uint32_t context_pointer,
    IWorldManagerRenderRuntime& runtime) {
    return RenderScene(manager+116u,context_pointer,runtime);
}
std::uint32_t WorldManagerRenderer::RenderScene(
    const std::uint32_t scene, const std::uint32_t context_pointer,
    IWorldManagerRenderRuntime& runtime) {
    runtime.RenderWorld(scene, runtime.ReadWord(context_pointer));
    const auto update_context = runtime.ReadWord(context_pointer);
    const auto update_manager = runtime.ActorManager();
    runtime.ProcessActorUpdates(update_manager, update_context);

    const auto first_clump = runtime.ReadWord(scene + 132u);
    if (first_clump != 0u) runtime.RenderClump(first_clump);
    const auto second_clump = runtime.ReadWord(scene + 112u);
    if (second_clump != 0u) runtime.RenderClump(second_clump);

    runtime.SortAtomicGeometries(scene + 212u);
    runtime.SortAtomicGeometries(scene + 196u);
    const auto component_context = runtime.ReadWord(context_pointer);
    const auto component_manager = runtime.ActorManager();
    return runtime.UpdateActorComponents(component_manager, component_context);
}

}  // namespace pal4
