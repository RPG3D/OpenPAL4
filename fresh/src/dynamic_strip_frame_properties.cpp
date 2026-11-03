#include "pal4/dynamic_strip_object_config.h"

#include <bit>
#include <cstdint>

namespace pal4 {
namespace {

float ReadFloat(const StripVisualBlockView& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

DynamicStripVector ReadVector(const StripVisualBlockView& state,
                              std::uint32_t offset) {
    return {ReadFloat(state, offset), ReadFloat(state, offset + 4u),
            ReadFloat(state, offset + 8u)};
}

}  // namespace

void DynamicStripObjectConfiguration::UpdateFrameProperties(
    StripVisualBlockView state, IStripVisualFrameRuntime& runtime) {
    const auto frame = state.words[14u];
    if (frame == 0u || (state.words[0u] >> 24u) != 0u) return;

    for (std::uint32_t offset = 16u; offset <= 24u; offset += 4u) {
        if (ReadFloat(state, offset) < 0.0f) state.words[offset / 4u] = 0u;
    }
    runtime.SetFrameIdentity(frame);
    runtime.ScaleFrame(frame, ReadVector(state, 16u), 0);
    runtime.RotateFrame(frame, {1.0f, 0.0f, 0.0f},
                        ReadFloat(state, 28u), 2);
    runtime.RotateFrame(frame, {0.0f, 1.0f, 0.0f},
                        ReadFloat(state, 32u), 2);
    runtime.RotateFrame(frame, {0.0f, 0.0f, 1.0f},
                        ReadFloat(state, 36u), 2);
    runtime.TranslateFrame(frame, ReadVector(state, 40u), 2);
    runtime.UpdateFrameObjects(frame);
}

void DynamicStripObjectConfiguration::UpdateFrameProperties(
    DynamicStripObjectState& s,IDynamicStripVisualRuntime& r) {
    UpdateFrameProperties(StripVisualBlockView{std::span{s.words}.first<34u>()},r);
}
}  // namespace pal4
