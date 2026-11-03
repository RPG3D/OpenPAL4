#include "pal4/cylinder_object_update.h"

#include <bit>

namespace pal4 {

void CylinderObjectUpdater::Update(
    CylinderObjectState& state, const double delta_time,
    ICylinderObjectUpdateRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 16u) & 0xffu) == 0u)
        return;

    if ((state.words[2u] >> 24u) != 0u) {
        auto color = runtime.QueryTimedFacet(state, 44u);
        auto count = runtime.Ftol(
            static_cast<double>(color[3u]) -
            static_cast<double>(std::bit_cast<float>(state.words[24u])) *
                delta_time);
        if (count < 0)
            count = 0;
        color[3u] = static_cast<std::uint8_t>(count);
        runtime.ApplyTimedFacet(state, 44u, color);
    }

}

}  // namespace pal4
