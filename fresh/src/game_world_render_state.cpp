#include "pal4/game_world_render_state.h"

namespace pal4 {

void GameWorldRenderState::Apply(const std::uint32_t world,
                                 IGameWorldRenderStateRuntime& runtime) {
    const auto settings = runtime.ReadWord(world + 0x1f8u);
    if (settings == 0u || runtime.ReadByte(settings) == 0u) return;
    const auto device = runtime.GetD3DDevice();
    if (device == 0u) return;

    if (runtime.ReadByte(settings + 1u) == 0u) {
        runtime.SetRenderState(device, 28u, 0u);
        return;
    }

    const auto mode = runtime.ReadWord(settings + 8u);
    runtime.SetRenderState(device, 35u, mode);
    runtime.SetRenderState(device, 140u, 0u);
    runtime.SetRenderState(device, 48u, 0u);
    runtime.SetRenderState(device, 34u, runtime.ReadWord(settings + 4u));
    runtime.SetRenderState(device, 28u, 1u);
    if (runtime.ReadWord(settings + 8u) == 3u) {
        runtime.SetRenderState(device, 36u, runtime.ReadWord(settings + 12u));
        runtime.SetRenderState(device, 37u, runtime.ReadWord(settings + 16u));
    } else {
        runtime.SetRenderState(device, 38u, runtime.ReadWord(settings + 20u));
    }
}

void GameWorldRenderState::Clear(const std::uint32_t world,
                                 IGameWorldRenderStateRuntime& runtime) {
    const auto settings = runtime.ReadWord(world + 0x1f8u);
    if (settings == 0u || runtime.ReadByte(settings) == 0u) return;
    const auto device = runtime.GetD3DDevice();
    if (device == 0u) return;

    runtime.SetRenderState(device, 28u, 0u);
    runtime.SetRenderState(device, 35u, 0u);
    runtime.SetRenderState(device, 140u, 0u);
    runtime.SetRenderState(device, 48u, 0u);
}

}  // namespace pal4
