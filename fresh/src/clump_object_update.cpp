#include "pal4/clump_object_update.h"

#include <bit>
#include <cmath>

namespace pal4 {

void ClumpObject::Update(ClumpObjectState& state,
                         const double delta_time,
                         IClumpObjectUpdateRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 16u) & 0xffu) == 0u)
        return;

    if ((state.words[2u] >> 24u) != 0u) {
        auto color = runtime.QueryTimedFacet(state, 36u);
        auto count = runtime.Ftol(static_cast<double>(color[3u]) -
                                  static_cast<double>(
                                      std::bit_cast<float>(state.words[43u])) *
                                      delta_time);
        if (count < 0)
            count = 0;
        color[3u] = static_cast<std::uint8_t>(count);
        runtime.ApplyTimedFacet(state, 36u, color);
    }

    const auto manager = state.words[18u];
    const auto frame_time = static_cast<float>(delta_time);
    runtime.UpdateManager(manager, frame_time);
    if (state.words[19u] == 0u || (state.words[42u] & 0xffu) != 0u)
        return;

    const auto clump = runtime.ReadWord(manager + 20u);
    const auto hierarchy = runtime.FindSkinHierarchy(clump);
    if (hierarchy == 0u)
        return;

    const auto interpolator = runtime.ReadWord(hierarchy + 32u);
    const auto animation = runtime.ReadWord(interpolator);
    const auto duration = runtime.ReadFloat(animation + 12u);
    const auto current_time = runtime.ReadFloat(interpolator + 4u);
    const auto next_time = static_cast<double>(frame_time) +
                           static_cast<double>(current_time);
    if (next_time < static_cast<double>(duration) ||
        std::isunordered(next_time, static_cast<double>(duration))) {
        runtime.AddAnimationTime(interpolator, frame_time);
    } else {
        runtime.AddAnimationTime(
            interpolator,
            static_cast<float>(static_cast<double>(duration) - next_time));
        runtime.SetAnimationTime(interpolator, 0.0f);
    }
    runtime.UpdateHierarchyMatrices(hierarchy);
}

}  // namespace pal4
