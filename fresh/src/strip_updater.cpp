#include "pal4/strip_updater.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t Byte(const StripObjectState& state, std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

float Float(const StripObjectState& state, std::uint32_t offset) {
    return std::bit_cast<float>(state.words[offset / 4u]);
}

void WriteFloat(StripObjectState& state, std::uint32_t offset, float value) {
    state.words[offset / 4u] = std::bit_cast<std::uint32_t>(value);
}

}  // namespace

void StripUpdater::Advance(StripObjectState& state, const double delta,
                           IStripUpdateRuntime& runtime) {
    if (Byte(state, 4u) == 0u || Byte(state, 6u) == 0u)
        return;
    if (Byte(state, 11u) != 0u) {
        const auto sample = runtime.ReadPlayback(state, 44u);
        auto remaining = runtime.FloatToInt(
            static_cast<double>(sample.remaining) -
            static_cast<double>(Float(state, 144u)) * delta);
        if (remaining < 0)
            remaining = 0;
        runtime.WritePlayback(state, 44u, {
            sample.red, sample.green, sample.blue,
            static_cast<std::uint8_t>(remaining)});
    }
    if (Byte(state, 140u) == 0u || Byte(state, 160u) == 0u)
        return;
    const auto elapsed = static_cast<double>(Float(state, 192u)) + delta;
    WriteFloat(state, 192u, static_cast<float>(elapsed));
    if (!(elapsed >= static_cast<double>(Float(state, 164u))))
        return;

    auto column = static_cast<std::int32_t>(state.words[44u]) + 1;
    if (column >= static_cast<std::int32_t>(state.words[42u])) {
        column = 0;
        auto row = static_cast<std::int32_t>(state.words[45u]) - 1;
        if (row < 0)
            row = static_cast<std::int32_t>(state.words[43u]) - 1;
        state.words[45u] = static_cast<std::uint32_t>(row);
    }
    state.words[44u] = static_cast<std::uint32_t>(column);
    const auto u = static_cast<float>(column) * Float(state, 184u);
    const auto v = static_cast<float>(
        static_cast<std::int32_t>(state.words[45u])) * Float(state, 188u);
    WriteFloat(state, 196u, u);
    WriteFloat(state, 200u, v);
    WriteFloat(state, 204u, u + Float(state, 184u));
    WriteFloat(state, 208u, v + Float(state, 188u));
    runtime.UpdateGeometryVertices(state.words[38u], {
        Float(state, 196u), Float(state, 200u),
        Float(state, 204u), Float(state, 208u)});
    state.words[48u] = 0u;
}

}  // namespace pal4
