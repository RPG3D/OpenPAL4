#include "pal4/game_world_material_params.h"

#include <array>
#include <cstdint>

namespace pal4 {

std::uint32_t GameWorldMaterialParams::Load(
    GameWorldObjectState& state, IGameWorldMaterialRuntime& runtime) {
    const auto path = runtime.ReadText(state.words[1]) + "world.flr";
    const auto stream = runtime.OpenDataStream(2u, 1u, path);
    if (stream == 0u)
        return 0u;
    for (const auto offset : {10u, 17u, 22u, 27u, 32u, 41u})
        runtime.ReadData(stream, state, offset, 4u);
    return runtime.CloseDataStream(stream, 0u);
}

}  // namespace pal4
