#include "pal4/thunder_visual_transform.h"

#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

float ReadFloat(const ThunderObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.words[(offset + 60u) / 4u]);
}

ThunderVector3 ReadVector(const ThunderObjectState& state,
                          std::uint32_t offset) {
    return {ReadFloat(state, offset), ReadFloat(state, offset + 4u),
            ReadFloat(state, offset + 8u)};
}

}  // namespace

std::uint32_t ThunderVisualTransform::SetupSourceFrame(
    ThunderObjectState& state, IThunderSourceFrameRuntime& runtime) {
    const auto frame = state.words[28u];
    if (frame == 0u) return 0u;

    for (std::uint32_t offset = 12u; offset <= 20u; offset += 4u) {
        if (ReadFloat(state, offset) < 0.0f)
            state.words[(offset + 60u) / 4u] = 0u;
    }
    runtime.FrameSetIdentity(frame);
    runtime.ScaleFrame(frame, ReadVector(state, 12u), 0u);
    runtime.RotateFrame(frame, {1.0f, 0.0f, 0.0f},
                        ReadFloat(state, 24u), 2u);
    runtime.RotateFrame(frame, {0.0f, 1.0f, 0.0f},
                        ReadFloat(state, 28u), 2u);
    runtime.RotateFrame(frame, {0.0f, 0.0f, 1.0f},
                        ReadFloat(state, 32u), 2u);
    runtime.TranslateFrame(frame, ReadVector(state, 36u), 2u);
    return runtime.FrameUpdateObjects(frame);
}

}  // namespace pal4
