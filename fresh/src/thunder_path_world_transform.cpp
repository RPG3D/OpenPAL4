#include "pal4/thunder_visual_path.h"

#include <bit>
#include <cstdint>

namespace pal4 {

void ThunderVisualPath::TransformToWorld(
    ThunderObjectState& state, IThunderPathTransformRuntime& runtime) {
    const auto transform = runtime.FrameTransform(state.words[34u]);
    const auto begin = state.words[47u];
    const auto end = state.words[48u];
    if (begin == 0u ||
        static_cast<std::int32_t>(end - begin) / 24 <= 0)
        return;

    auto segments = runtime.ReadSegments(begin, end);
    const ThunderVector3 midpoint{
        std::bit_cast<float>(state.words[31u]),
        std::bit_cast<float>(state.words[32u]),
        std::bit_cast<float>(state.words[33u])};
    for (auto& segment : segments) {
        segment.first = runtime.TransformVector(segment.first, transform);
        segment.second = runtime.TransformVector(segment.second, transform);
        for (std::uint32_t axis = 0u; axis < 3u; ++axis) {
            segment.first[axis] = static_cast<float>(
                static_cast<double>(midpoint[axis]) + segment.first[axis]);
            segment.second[axis] = static_cast<float>(
                static_cast<double>(midpoint[axis]) + segment.second[axis]);
        }
    }
    runtime.WriteSegments(state, segments);
}

}  // namespace pal4
