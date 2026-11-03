#include "pal4/dynamic_strip_object_update.h"

namespace pal4 {

std::uint8_t DynamicStripObjectUpdater::Update(
    DynamicStripObjectState& state, const double frame_time,
    IDynamicStripObjectUpdateRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    const auto enabled = static_cast<std::uint8_t>(state.words[1u] >> 16u);
    if (enabled == 0u)
        return enabled;
    const auto mode = static_cast<std::uint8_t>(state.words[2u] >> 24u);
    return runtime.UpdateVisual(state, frame_time, mode);
}

}  // namespace pal4
