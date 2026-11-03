#include "pal4/game_world_operations.h"

#include <array>
#include <cstdint>

namespace pal4 {

bool GameWorldOperations::AddLight(
    GameWorldObjectState& state,
    const std::array<std::uint32_t, 4>& rgba_bits,
    IGameWorldOperationsRuntime& runtime) {
    if (state.words[12] == 0u)
        return false;
    runtime.DestroyLight(state.words[5]);
    state.words[5] = runtime.CreateLight();
    if (state.words[5] == 0u)
        return false;
    runtime.SetLightColor(state.words[5], rgba_bits);
    runtime.SetLightRadius(state.words[5], kLightRadiusBits);
    runtime.SetLightConeAngle(state.words[5], kLightConeAngleBits);
    runtime.AddLight(state.words[4], state.words[5]);
    return true;
}

bool GameWorldOperations::SetCamera(GameWorldObjectState& state,
                                     const std::uint32_t camera,
                                     IGameWorldOperationsRuntime& runtime) {
    return SetCameraBinding(state,camera,runtime);
}
bool GameWorldOperations::SetCameraBinding(GameWorldObjectState& state,
    const std::uint32_t camera,IGameWorldCameraRuntime& runtime) {
    if (state.words[12] == 0u)
        return false;
    if (state.words[4] != 0u && state.words[11] != 0u)
        runtime.RemoveCamera(state.words[4], state.words[11]);
    state.words[11] = camera;
    runtime.AddCamera(state.words[4], camera);
    return true;
}

bool GameWorldOperations::AddClump(const GameWorldObjectState& state,
                                    const std::uint32_t clump,
                                    IGameWorldOperationsRuntime& runtime) {
    if (state.words[12] == 0u || clump == 0u)
        return false;
    const auto owner = runtime.GetClumpWorld(clump);
    if (owner != 0u && owner == state.words[4])
        return false;
    runtime.AddClump(state.words[4], clump);
    return true;
}

bool GameWorldOperations::RemoveClump(const GameWorldObjectState& state,
                                       const std::uint32_t clump,
                                       IGameWorldOperationsRuntime& runtime) {
    if (state.words[12] == 0u || clump == 0u)
        return false;
    const auto owner = runtime.GetClumpWorld(clump);
    if (owner == 0u || owner != state.words[4])
        return false;
    runtime.RemoveClump(owner, clump);
    return true;
}

}  // namespace pal4
