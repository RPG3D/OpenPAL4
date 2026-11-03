#include "pal4/game_world_scene_pass.h"

namespace pal4 {
namespace {

std::uint32_t RenderClump(const std::uint32_t address,
                          IGameWorldScenePassRuntime& runtime) {
    const auto clump = runtime.ReadWord(address);
    return clump == 0u
               ? 0u
               : runtime.Call(GameWorldScenePassOperation::RenderClump, clump);
}

}  // namespace

std::uint32_t GameWorldScenePass::RenderPrimaryClump(
    const std::uint32_t world, IGameWorldScenePassRuntime& runtime) {
    return RenderClump(world + 112u, runtime);
}

std::uint32_t GameWorldScenePass::RenderSecondaryClump(
    const std::uint32_t world, IGameWorldScenePassRuntime& runtime) {
    return RenderClump(world + 132u, runtime);
}

std::uint32_t GameWorldScenePass::Process(
    const std::uint32_t world, const std::uint32_t context,
    IGameWorldScenePassRuntime& runtime) {
    if (runtime.ReadWord(world + 48u) == 0u) return 0u;
    static_cast<void>(RenderSecondaryClump(world, runtime));
    static_cast<void>(runtime.Call(
        GameWorldScenePassOperation::UpdateArenaCache, world + 448u,
        context));
    static_cast<void>(RenderPrimaryClump(world, runtime));
    static_cast<void>(runtime.Call(
        GameWorldScenePassOperation::SortGeometry, world + 268u));
    static_cast<void>(runtime.Call(
        GameWorldScenePassOperation::SortGeometry, world + 300u));
    return runtime.Call(GameWorldScenePassOperation::SortGeometry,
                        world + 284u);
}

}  // namespace pal4
