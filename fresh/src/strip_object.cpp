#include "pal4/strip_object.h"

#include <array>
#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 10> kBaseFacets{
    0x84a1dcu, 0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1bcu, 0x84a1acu, 0x84a184u, 0x84a408u, 0x84a178u};
constexpr std::array<std::uint32_t, 10> kDerivedFacets{
    0x84aa84u, 0x84aa78u, 0x84aa6cu, 0x84aa60u, 0x84aa58u,
    0x84aa4cu, 0x84aa3cu, 0x84aa2cu, 0x84aa14u, 0x84aa08u};

void SetFacets(StripObjectState& state,
               const std::array<std::uint32_t, 10>& facets) {
    for (std::uint32_t i = 0u; i < facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(StripObjectState& state) {
    for (std::uint32_t i = 10u; i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void StoreByte(StripObjectState& state,
               const std::uint32_t offset, const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void ChangeInstanceCount(IStripObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

StripObjectState& StripObject::Construct(
    StripObjectState& state, IStripObjectRuntime& runtime) {
    state.words[0u] = 0x84b240u;
    for (std::uint32_t i = 1u; i < 5u; ++i)
        state.words[i] = 0u;
    SetFacets(state, kBaseFacets);
    state.words[33u] = 0u;
    state.words[34u] = 0u;
    StoreByte(state, 140u, 0u);
    state.words[37u] = 0u;
    state.words[38u] = 0u;
    state.words[39u] = 0u;
    StoreByte(state, 284u, runtime.ConstructorStackByte());
    state.words[72u] = 0u;
    state.words[73u] = 0u;
    state.words[74u] = 0u;
    state.words[0u] = 0x84aa90u;
    SetFacets(state, kDerivedFacets);
    try {
        ChangeInstanceCount(runtime, 1u);
    } catch (...) {
        RestoreBase(state);
        throw;
    }
    return state;
}

void StripObject::Destroy(
    StripObjectState& state, IStripObjectRuntime& runtime) {
    state.words[0u] = 0x84aa90u;
    SetFacets(state, kDerivedFacets);
    ChangeInstanceCount(runtime, 0xffffffffu);
    runtime.FreeBuffer(state.words[72u]);
    state.words[72u] = 0u;
    state.words[73u] = 0u;
    state.words[74u] = 0u;
    RestoreBase(state);
}

std::uintptr_t StripObject::DeletingDestroy(
    StripObjectState& state, const std::uint32_t flags,
    IStripObjectRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(state);
    return reinterpret_cast<std::uintptr_t>(&state);
}

std::string_view StripObject::ClassName() {
    return "StripObject";
}

void StripObject::SetEnabled(StripObjectState& state,
                             const std::uint8_t enabled,
                             IStripObjectPlaybackRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return;
    const auto previous = static_cast<std::uint8_t>(state.words[2u] >> 24u);
    if (previous == enabled)
        return;
    StoreByte(state, 11u, enabled);
    if (enabled == 0u)
        return;
    const auto frame_count = runtime.StartFacet(state, 44u);
    const auto duration = runtime.Duration(state);
    const auto ratio = static_cast<float>(frame_count) / duration;
    state.words[36u] = std::bit_cast<std::uint32_t>(static_cast<float>(ratio));
}

}  // namespace pal4
