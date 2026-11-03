#include "pal4/sequence_anim_updater.h"

#include <bit>

namespace pal4 {
namespace {

float ReadFloat(const SequenceAnimObjectState& state,
                const std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}
void StoreFloat(SequenceAnimObjectState& state,
                const std::uint32_t offset, const float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

}  // namespace

void SequenceAnimUpdater::Update(
    SequenceAnimObjectState& state, const double elapsed,
    ISequenceAnimUpdateRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u ||
        ((state.words[1u] >> 16u) & 0xffu) == 0u)
        return;
    const auto interval = ReadFloat(state, 52u);
    if (!(interval > 0.0f))
        return;

    if (((state.words[2u] >> 24u) & 0xffu) != 0u) {
        const auto color = runtime.ReadColor(state);
        const auto alpha = static_cast<std::uint8_t>(color >> 24u);
        const auto progress = ReadFloat(state, 116u);
        auto next_alpha = runtime.Ftol(
            static_cast<double>(alpha) -
            static_cast<double>(progress) * elapsed);
        if (next_alpha < 0)
            next_alpha = 0;
        runtime.SetColor(state, (color & 0x00ffffffu) |
                                (static_cast<std::uint32_t>(
                                     static_cast<std::uint8_t>(next_alpha)) << 24u));
    }

    const auto elapsed_in_frame =
        static_cast<double>(ReadFloat(state, 80u)) + elapsed;
    StoreFloat(state, 80u, static_cast<float>(elapsed_in_frame));
    if (elapsed_in_frame >= static_cast<double>(interval)) {
        state.words[16u] += 1u;
        auto column = static_cast<std::int32_t>(state.words[16u]);
        if (column >= static_cast<std::int32_t>(state.words[14u])) {
            column = 0;
            state.words[16u] = 0u;
            state.words[17u] -= 1u;
            auto row = static_cast<std::int32_t>(state.words[17u]);
            if (row < 0)
                row = static_cast<std::int32_t>(state.words[15u]) - 1;
            state.words[17u] = static_cast<std::uint32_t>(row);
        }
        const auto u_step = ReadFloat(state, 72u);
        const auto v_step = ReadFloat(state, 76u);
        const auto u = static_cast<double>(column) * u_step;
        const auto v = static_cast<double>(
            static_cast<std::int32_t>(state.words[17u])) * v_step;
        StoreFloat(state, 84u, static_cast<float>(u));
        StoreFloat(state, 88u, static_cast<float>(v));
        StoreFloat(state, 92u, static_cast<float>(u + u_step));
        StoreFloat(state, 96u, static_cast<float>(v + v_step));
        const auto resource = state.words[30u];
        runtime.UpdateGeometry(runtime.Geometry(resource), {
            state.words[21u], state.words[22u],
            state.words[23u], state.words[24u]});
        state.words[20u] = 0u;
    }

    const auto resource = state.words[30u];
    runtime.UpdateState(resource, elapsed,
                        runtime.SystemResource(state.words[4u]));
}

}  // namespace pal4
