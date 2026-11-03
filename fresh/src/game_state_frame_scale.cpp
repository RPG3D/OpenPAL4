#include "pal4/game_state_frame_scale.h"

namespace pal4 {

std::uint32_t GameStateFrameScale::SetAndReset(
    ActorManagerEffectHandle handle, const float scale,
    IGameStateFrameScaleRuntime& runtime) {
    runtime.WriteFloat(runtime.GameState() + kScaleOffset, scale);
    return runtime.ResetEffectHandle(handle);
}

}  // namespace pal4
