#include "pal4/effect_object_playback_facet.h"

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

}  // namespace

std::uint8_t EffectObjectPlaybackFacet::Pause(
    const EffectObjectState& state,
    IEffectObjectPlaybackFacetRuntime& runtime) {
    if (GetByte(state, 6u) == 0u || GetByte(state, 4u) == 0u)
        return 0u;
    const auto instance = state.words[23u];
    const auto resource = runtime.ReadWord(instance + 8u);
    if (resource != 0u)
        return static_cast<std::uint8_t>(resource);
    const auto pending = runtime.ReadByte(instance + 81u);
    if (pending == 0u)
        return 0u;
    const auto retry_resource = runtime.ReadWord(instance + 8u);
    if (retry_resource != 0u)
        runtime.WriteByte(instance + 82u, 1u);
    return static_cast<std::uint8_t>(retry_resource);
}

std::uint8_t EffectObjectPlaybackFacet::Resume(
    const EffectObjectState& state,
    IEffectObjectPlaybackFacetRuntime& runtime) {
    if (GetByte(state, 6u) == 0u || GetByte(state, 4u) == 0u)
        return 0u;
    const auto instance = state.words[23u];
    const auto resource = runtime.ReadWord(instance + 8u);
    if (resource == 0u)
        return 0u;
    const auto paused = runtime.ReadByte(instance + 82u);
    if (paused != 0u)
        runtime.WriteByte(instance + 82u, 0u);
    return paused;
}

std::uint32_t EffectObjectPlaybackFacet::Clear(
    EffectObjectState& state, const std::uint32_t entry_eax,
    IEffectObjectPlaybackFacetRuntime& runtime) {
    if (GetByte(state, 4u) == 0u)
        return entry_eax;
    const auto instance = state.words[23u];
    if (instance != 0u) {
        runtime.ResetInstance(instance);
        runtime.WriteByte(instance + 4u, 1u);
        runtime.WriteByte(instance + 80u, 0u);
    }
    SetByte(state, 88u, 0u);
    SetByte(state, 6u, 0u);
    SetByte(state, 7u, 0u);
    return instance == 0u ? entry_eax : instance;
}

double EffectObjectMetric::GetProgress(
    EffectObjectState& state, IEffectObjectMetricRuntime& runtime) {
    if (GetByte(state, 4u) == 0u || GetByte(state, 6u) == 0u)
        return 0.0;
    runtime.Prime(state, 1u);
    return runtime.Read(state);
}

void EffectObjectFacetA718::Initialize(
    EffectObjectFacetA718Storage& facet) {
    facet.vtable = kVtable;
}

std::uintptr_t EffectObjectFacetA718::DeletingDestroy(
    EffectObjectFacetA718Storage& facet, const std::uint32_t flags,
    IEffectObjectFacetA718Runtime& runtime) {
    facet.vtable = kVtable;
    if ((flags & 1u) != 0u)
        runtime.FreeFacet(facet);
    return reinterpret_cast<std::uintptr_t>(&facet);
}

}  // namespace pal4
