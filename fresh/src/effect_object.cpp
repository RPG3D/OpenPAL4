#include "pal4/effect_object.h"

#include <bit>
#include <cstddef>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 8> kBaseFacets{
    0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a718u, 0x84a1bcu, 0x84a178u, 0x84a408u};
constexpr std::array<std::uint32_t, 8> kDerivedFacets{
    0x84a6b4u, 0x84a6a8u, 0x84a69cu, 0x84a694u,
    0x84a67cu, 0x84a670u, 0x84a664u, 0x84a64cu};

void SetFacets(EffectObjectState& state,
               const std::array<std::uint32_t, 8>& facets) {
    for (std::size_t i = 0u; i < facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(EffectObjectState& state) {
    for (std::size_t i = kBaseFacets.size(); i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void UpdateInstanceCount(IEffectObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

EffectObjectState& EffectObject::Construct(
    EffectObjectState& state, IEffectObjectRuntime& runtime) {
    state.words[0u] = 0x84b240u;
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    state.words[4u] = 0u;
    SetFacets(state, kBaseFacets);
    state.words[22u] &= 0xffffff00u;
    state.words[24u] = 0u;
    state.words[0u] = 0x84a6c0u;
    SetFacets(state, kDerivedFacets);
    try {
        UpdateInstanceCount(runtime, 1u);
    } catch (...) {
        RestoreBase(state);
        throw;
    }
    return state;
}

void EffectObject::Destroy(EffectObjectState& state,
                           IEffectObjectRuntime& runtime) {
    state.words[0u] = 0x84a6c0u;
    SetFacets(state, kDerivedFacets);
    UpdateInstanceCount(runtime, 0xffffffffu);
    RestoreBase(state);
}

std::uintptr_t EffectObject::DeletingDestroy(
    EffectObjectState& state, const std::uint32_t flags,
    IEffectObjectRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(state);
    return reinterpret_cast<std::uintptr_t>(&state);
}

std::uint32_t EffectObject::ResourceHandle(
    const EffectObjectState& state) {
    return state.words[24u];
}

std::string_view EffectObject::ClassName() {
    return "EffectObject";
}

float EffectObject::GetDuration(const EffectObjectState& state) {
    return std::bit_cast<float>(state.words[25u]);
}

void EffectObjectPlayback::SetEnabled(
    EffectObjectState& state, const std::uint8_t enabled,
    IEffectObjectPlaybackRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return;
    state.words[2u] = (state.words[2u] & 0x00ffffffu) |
                      (static_cast<std::uint32_t>(enabled) << 24u);
    state.words[25u] = enabled == 0u
        ? 0u
        : std::bit_cast<std::uint32_t>(static_cast<float>(
              runtime.ResetAndMeasure(state.words[23u], 0u)));
}

std::uint32_t EffectObjectTransform::ForwardFacet28(
    EffectObjectState& state,
    const std::array<std::uint32_t, 3>& values,
    IEffectObjectTransformRuntime& runtime) {
    return runtime.CallFacet28Slot8(state, values, 0u);
}

}  // namespace pal4
