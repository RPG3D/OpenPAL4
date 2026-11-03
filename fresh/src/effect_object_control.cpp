#include "pal4/effect_object_control.h"

#include <array>

namespace pal4 {
namespace {

std::uint8_t GetByte(const EffectObjectState& state,
                     const std::uint32_t offset) {
    return static_cast<std::uint8_t>(state.words[offset / 4u] >>
                                     ((offset % 4u) * 8u));
}

void SetByte(EffectObjectState& state, const std::uint32_t offset,
             const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

constexpr std::array<std::uint32_t, 9u> kStoppedTransform{
    0x3f800000u, 0x3f800000u, 0x3f800000u,
    0u, 0u, 0u,
    0u, 0x3f800000u, 0x3f800000u};

}  // namespace

bool EffectObjectControl::Reset(EffectObjectState& state,
                                IEffectObjectControlRuntime& runtime) {
    if (GetByte(state, 4u) == 0u)
        return false;

    runtime.ResetPlaybackFacet(state);
    SetByte(state, 11u, 0u);
    for (std::uint32_t i = 0u; i != 9u; ++i)
        state.words[13u + i] = state.words[33u + i];

    const auto instance = state.words[23u];
    if (instance == 0u)
        return false;

    runtime.ResetInstance(instance);
    runtime.WriteWord(instance + 12u, state.words[4u]);
    runtime.WriteByte(instance + 4u, 1u);
    runtime.WriteByte(instance + 80u, 0u);
    runtime.WriteByte(instance + 83u,
                      runtime.ReadByte(state.words[3u] + 83u));
    runtime.UpdateFrame(state);
    return runtime.FinalizeInstance(instance);
}

std::uint32_t EffectObjectControl::Stop(
    EffectObjectState& state, const std::uint32_t context,
    IEffectObjectControlRuntime& runtime) {
    if (GetByte(state, 4u) != 0u) {
        runtime.ResetPlaybackFacet(state);
        SetByte(state, 4u, 0u);
        SetByte(state, 6u, 0u);
        SetByte(state, 7u, 0u);
        for (std::uint32_t i = 0u; i != kStoppedTransform.size(); ++i)
            state.words[13u + i] = kStoppedTransform[i];
        SetByte(state, 88u, 0u);

        const auto instance = state.words[23u];
        if (instance != 0u) {
            runtime.CleanupInstance(instance, context);
            runtime.DestroyInstance(instance, context);
            state.words[23u] = 0u;
        }
        const auto frame = state.words[24u];
        if (frame != 0u) {
            runtime.DestroyFrame(frame);
            state.words[24u] = 0u;
        }
        runtime.ProcessStop(state.words[4u], state);
    }
    return runtime.DeleteObject(state, 1u);
}

}  // namespace pal4
