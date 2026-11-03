#include "pal4/game_world_light_gather.h"

namespace pal4 {

std::uint32_t GameWorldLightGather::Gather(
    const GameWorldObjectState& state,
    IGameWorldLightGatherRuntime& runtime) {
    const auto world = state.words[4];
    if (world == 0u)
        return 0u;
    return runtime.ForAllWorldSectors(
        world, WorldSectorCallback::GatherLight, 0u);
}

std::uint32_t GameWorldLightGather::GatherSectorMeshes(
    const std::uint32_t sector, const std::uint32_t user_data,
    IGameWorldLightGatherRuntime& runtime) {
    runtime.ForAllSectorMeshes(
        sector, WorldMeshCallback::SetTriStripMethod, user_data);
    return sector;
}

}  // namespace pal4
