#include "pal4/cylinder_object_color.h"

namespace pal4 {

std::uint8_t CylinderObjectColor::Get(
    const CylinderObjectState& state,
    std::array<std::uint8_t, 4>& output,
    ICylinderObjectColorRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i < output.size(); ++i)
        output[i] = runtime.ReadByte(state.words[22u] + 48u + i);
    return output[3u];
}

std::uint8_t CylinderObjectColor::Set(
    const CylinderObjectState& state,
    const std::array<std::uint8_t, 4>& color,
    ICylinderObjectColorRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.SetColor(state.words[22u], color);
}

}  // namespace pal4
