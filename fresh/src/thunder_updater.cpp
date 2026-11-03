#include "pal4/thunder_updater.h"

#include <bit>

namespace pal4 {

std::uint32_t ThunderUpdater::Update(
    ThunderObjectState& state, const double elapsed,
    const std::uint32_t entry_eax, IThunderUpdateRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 16u) & 0xffu) == 0u)
        return entry_eax;

    if ((state.words[2u] >> 24u) != 0u) {
        std::array<std::uint32_t, 4u> outputs{};
        runtime.ReadPlayback(state, outputs);
        const auto rate = std::bit_cast<float>(state.words[59u]);
        auto remaining = runtime.FloatToInt(
            static_cast<double>(static_cast<std::int32_t>(outputs[2u])) -
            static_cast<double>(rate) * elapsed);
        if (remaining < 0)
            remaining = 0;
        runtime.WritePlayback(state, {
            runtime.StackByte(), static_cast<std::uint8_t>(outputs[0u]),
            static_cast<std::uint8_t>(outputs[1u]),
            static_cast<std::uint8_t>(remaining)});
    }

    if ((state.words[58u] & 0xffu) == 0u && state.words[3u] != 0u) {
        std::array<std::uint32_t, 3u> target{};
        runtime.ResolveTarget(state, state.words[3u], target);
        for (std::uint32_t i = 0u; i < 3u; ++i)
            state.words[37u + i] = target[i];
    }
    return runtime.AdvanceVisual(state, elapsed);
}

}  // namespace pal4
