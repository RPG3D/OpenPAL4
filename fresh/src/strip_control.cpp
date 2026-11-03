#include "pal4/strip_control.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t Byte(const StripObjectState& state, std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteByte(StripObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

std::array<float, 3> Vector(const StripObjectState& state,
                            const std::uint32_t offset) {
    return {std::bit_cast<float>(state.words[offset / 4u]),
            std::bit_cast<float>(state.words[offset / 4u + 1u]),
            std::bit_cast<float>(state.words[offset / 4u + 2u])};
}

}  // namespace

std::uint8_t StripControl::Reset(
    StripObjectState& state, IStripControlRuntime& runtime) {
    if (Byte(state, 4u) == 0u)
        return 0u;
    state.words[25u] = state.words[57u];
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        state.words[19u + i] = state.words[61u + i];
        state.words[16u + i] = state.words[58u + i];
        state.words[22u + i] = state.words[64u + i];
    }
    if (Byte(state, 140u) != 0u) {
        const auto model = state.words[38u];
        runtime.SetColor(model, state.words[25u]);
        runtime.SetRotation(model, Vector(state, 76u));
        runtime.SetScale(model, Vector(state, 64u));
        runtime.SetPosition(model, Vector(state, 88u));
        if (Byte(state, 160u) != 0u) {
            for (std::uint32_t i = 0u; i < 4u; ++i)
                state.words[49u + i] = state.words[67u + i];
            state.words[45u] = state.words[43u] - 1u;
            state.words[44u] = 0u;
            const std::array<float, 4> uv{
                std::bit_cast<float>(state.words[49u]),
                std::bit_cast<float>(state.words[50u]),
                std::bit_cast<float>(state.words[51u]),
                std::bit_cast<float>(state.words[52u])};
            runtime.UpdateGeometryVertices(model, uv);
        }
    } else {
        runtime.UpdateFrameProperties(state);
    }
    WriteByte(state, 11u, 0u);
    return 1u;
}

std::uint32_t StripControl::Stop(
    StripObjectState& state, IStripStopRuntime& runtime) {
    if (Byte(state, 4u) != 0u) {
        runtime.StopFacet(state, 48u);
        runtime.StopFacet(state, 20u);
        WriteByte(state, 4u, 0u);
        WriteByte(state, 6u, 0u);
        WriteByte(state, 7u, 0u);
        if (state.words[39u] != 0u) {
            const auto document = state.words[39u];
            runtime.DestroyDocument(document);
            runtime.FreeMemory(document);
            state.words[39u] = 0u;
        }
        if (state.words[37u] != 0u) {
            const auto atomic = state.words[37u];
            runtime.DestroyAtomic(atomic);
            runtime.FreeMemory(atomic);
            state.words[37u] = 0u;
        }
        if (state.words[38u] != 0u) {
            const auto model = state.words[38u];
            runtime.DestroyStraight(model);
            runtime.FreeMemory(model);
            state.words[38u] = 0u;
        }
        if (Byte(state, 140u) != 0u) {
            state.words[34u] = 0u;
        } else {
            if (state.words[34u] != 0u) {
                runtime.DestroyClump(state.words[34u]);
                state.words[34u] = 0u;
            }
            if (state.words[33u] != 0u)
                runtime.DestroyFrame(state.words[33u]);
        }
        state.words[33u] = 0u;
        runtime.ProcessEvent(state.words[4u], state);
    }
    return runtime.DeleteObject(state, 1u);
}

}  // namespace pal4
