#include "pal4/game_world_renderer.h"

#include <array>

namespace pal4 {

bool GameWorldRenderer::Render(const GameWorldObjectState& state,
                               IGameWorldDrawRuntime& runtime) {
    if (state.words[12] == 0u)
        return false;
    runtime.RenderWorld(state.words[4]);
    return true;
}

bool GameWorldRenderer::UpdateRenderParameters(
    GameWorldObjectState& state, const std::uint32_t delta_bits,
    IGameWorldRenderRuntime& runtime) {
    if (state.words[12] == 0u)
        return false;
    for (const auto offset : std::array<std::uint32_t, 6>{7u, 14u, 19u,
                                                            24u, 29u, 34u})
        runtime.UpdateUvAnimation(state, offset, delta_bits);
    return true;
}

}  // namespace pal4
