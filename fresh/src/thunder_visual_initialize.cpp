#include "pal4/thunder_visual_storage.h"

#include <bit>
#include <cstdint>

namespace pal4 {

std::uint8_t ThunderVisualStorage::InitializeWithTexture(
    ThunderObjectState& state, std::uint32_t texture, float fps,
    std::uint8_t mode, IThunderVisualInitializationRuntime& runtime) {
    runtime.ResetProperties(state);
    if (texture == 0u) return 0u;

    state.words[29u] = texture;
    runtime.WriteWord(texture + 84u,
                      runtime.ReadWord(texture + 84u) + 1u);
    state.words[35u] = fps > 0.0f
                           ? std::bit_cast<std::uint32_t>(
                                 static_cast<float>(1.0 / static_cast<double>(fps)))
                           : 0u;
    state.words[36u] = 0u;
    state.words[28u] = runtime.CreateFrame();
    runtime.SetupAndUpdateFrame(state);
    state.words[34u] = runtime.CreateFrame();
    runtime.SetFrameIdentity(state.words[34u]);
    state.words[15u] = (state.words[15u] & 0xff00ff00u) |
                      (static_cast<std::uint32_t>(mode) << 16u) |
                      1u;
    return 1u;
}

}  // namespace pal4
