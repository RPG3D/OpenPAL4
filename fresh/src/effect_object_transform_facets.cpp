#include "pal4/effect_object_transform_facets.h"

#include <bit>

namespace pal4 {
namespace {

std::uint32_t SetTriple(
    EffectObjectState& state, const std::uint32_t first_word,
    const std::array<float, 3u>& values, const std::uint8_t relative,
    const std::uint32_t entry_eax,
    IEffectObjectTransformFacetRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return entry_eax & 0xffffff00u;
    for (std::uint32_t i = 0u; i != 3u; ++i) {
        const auto current = std::bit_cast<float>(state.words[first_word + i]);
        state.words[first_word + i] = std::bit_cast<std::uint32_t>(
            relative == 0u ? values[i] : current + values[i]);
    }
    return runtime.UpdateFrame(state);
}

}  // namespace

std::uint8_t EffectObjectTransformFacets::GetScale(
    const EffectObjectState& state,
    std::array<std::uint32_t, 3u>& output) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i != 3u; ++i)
        output[i] = state.words[13u + i];
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint32_t EffectObjectTransformFacets::SetScale(
    EffectObjectState& state, const std::array<float, 3u>& values,
    const std::uint8_t relative, const std::uint32_t entry_eax,
    IEffectObjectTransformFacetRuntime& runtime) {
    return SetTriple(state, 13u, values, relative, entry_eax, runtime);
}

std::uint32_t EffectObjectTransformFacets::SetRotation(
    EffectObjectState& state, const std::array<float, 3u>& values,
    const std::uint8_t relative, const std::uint32_t entry_eax,
    IEffectObjectTransformFacetRuntime& runtime) {
    return SetTriple(state, 16u, values, relative, entry_eax, runtime);
}

std::uint32_t EffectObjectTransformFacets::SetPosition(
    EffectObjectState& state, const std::array<float, 3u>& values,
    const std::uint8_t relative, const std::uint32_t entry_eax,
    IEffectObjectTransformFacetRuntime& runtime) {
    return SetTriple(state, 19u, values, relative, entry_eax, runtime);
}

std::uint32_t EffectObjectFrame::Rebuild(
    const EffectObjectState& state, IEffectObjectFrameRuntime& runtime) {
    const auto frame = state.words[24u];
    if (frame == 0u)
        return 0u;

    runtime.SetIdentity(frame);
    runtime.Scale(frame,
                  {state.words[13u], state.words[14u], state.words[15u]}, 0u);
    constexpr std::array<std::array<float, 3u>, 3u> axes{{
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}}};
    for (std::uint32_t i = 0u; i != 3u; ++i)
        runtime.Rotate(frame, axes[i],
                       std::bit_cast<float>(state.words[16u + i]), 2u);
    runtime.Translate(frame,
                      {state.words[19u], state.words[20u], state.words[21u]},
                      2u);
    return runtime.UpdateObjects(frame);
}

}  // namespace pal4
