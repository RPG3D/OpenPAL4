#include "pal4/effect_system_object_f_frame.h"

#include <bit>

namespace pal4 {

void EffectSystemObjectFFrame::Update(
    EffectSystemObjectFState& state, const double delta,
    IEffectSystemObjectFFrameRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    const auto running = static_cast<std::uint8_t>(state.words[1u] >> 16u);
    if (active == 0u || running == 0u)
        return;

    const auto timed = static_cast<std::uint8_t>(state.words[2u] >> 24u);
    if (timed != 0u) {
        std::array<std::int32_t, 4> task_values{};
        runtime.QueryTaskValues(state, task_values);
        const auto ratio = std::bit_cast<float>(state.words[30u]);
        auto remaining = runtime.FloatToLong(
            static_cast<double>(task_values[3u]) - ratio * delta);
        if (remaining < 0)
            remaining = 0;
        const std::array<std::uint8_t, 4> bytes{
            static_cast<std::uint8_t>(task_values[0u]),
            static_cast<std::uint8_t>(task_values[1u]),
            static_cast<std::uint8_t>(task_values[2u]),
            static_cast<std::uint8_t>(remaining)};
        runtime.WriteTaskBytes(state, bytes);
    }

    const auto delta_bits = std::bit_cast<std::uint64_t>(delta);
    const auto callback = runtime.ReadWord(state.words[4u] + 48u);
    runtime.UpdateObject(state.words[29u], static_cast<std::uint32_t>(delta_bits),
                         static_cast<std::uint32_t>(delta_bits >> 32u), callback);
}

}  // namespace pal4
