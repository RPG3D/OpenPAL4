#include "pal4/dynamic_strip_object.h"

#include <bit>
#include <cstddef>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 8> kBaseFacets{
    0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1bcu, 0x84a1dcu, 0x84a1acu, 0x84a178u};
constexpr std::array<std::uint32_t, 8> kDerivedFacets{
    0x84a5c0u, 0x84a5b4u, 0x84a5a8u, 0x84a5a0u,
    0x84a594u, 0x84a588u, 0x84a578u, 0x84a56cu};

void SetFacets(DynamicStripObjectState& state,
               const std::array<std::uint32_t, 8>& facets) {
    for (std::size_t i = 0u; i < facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(DynamicStripObjectState& state) {
    for (std::size_t i = kBaseFacets.size(); i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void WriteByte(DynamicStripObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

void UpdateInstanceCount(IDynamicStripObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

DynamicStripObjectState& DynamicStripObject::Construct(
    DynamicStripObjectState& state, const std::uint8_t first_byte,
    const std::uint8_t second_byte, IDynamicStripObjectRuntime& runtime) {
    state.words[0u] = 0x84b240u;
    state.words[1u] = 0u;
    state.words[2u] = 0u;
    state.words[3u] = 0u;
    state.words[4u] = 0u;
    SetFacets(state, kBaseFacets);
    WriteByte(state, 52u, 0u);
    WriteByte(state, 53u, 1u);
    WriteByte(state, 54u, 1u);
    WriteByte(state, 56u, 0u);
    state.words[27u] = 0u;
    state.words[28u] = 0u;
    state.words[29u] = 0u;
    state.words[38u] = 0u;
    state.words[39u] = 0u;
    WriteByte(state, 160u, first_byte);
    try {
        const auto sentinel = runtime.Allocate(36u);
        runtime.WriteWord(sentinel, sentinel);
        runtime.WriteWord(sentinel + 4u, sentinel);
        state.words[41u] = sentinel;
        state.words[42u] = 0u;
        WriteByte(state, 172u, second_byte);
        state.words[44u] = 0u;
        state.words[45u] = 0u;
        state.words[46u] = 0u;
        state.words[0u] = 0x84a5d0u;
        SetFacets(state, kDerivedFacets);
        UpdateInstanceCount(runtime, 1u);
    } catch (...) {
        runtime.CleanupResources(state);
        RestoreBase(state);
        throw;
    }
    return state;
}

float DynamicStripObject::GetLife(
    const DynamicStripObjectState& state) {
    return std::bit_cast<float>(state.words[28u]);
}

std::uint32_t DynamicStripObject::ResourceHandle(
    const DynamicStripObjectState& state) {
    return (state.words[1u] & 0xffu) == 0u ? 0u : state.words[27u];
}

std::string_view DynamicStripObject::ClassName() {
    return "DynamicStripObject";
}

std::uint8_t DynamicStripObject::SetModeByte(
    DynamicStripObjectState& state, const std::uint8_t mode) {
    WriteByte(state, 11u, mode);
    return mode;
}

void DynamicStripObject::Destroy(DynamicStripObjectState& state,
                                 IDynamicStripObjectRuntime& runtime) {
    state.words[0u] = 0x84a5d0u;
    SetFacets(state, kDerivedFacets);
    UpdateInstanceCount(runtime, 0xffffffffu);
    runtime.CleanupResources(state);
    runtime.ReleaseRange(state.words[44u], state.words[45u]);
    runtime.Free(state.words[44u]);
    state.words[44u] = 0u;
    state.words[45u] = 0u;
    state.words[46u] = 0u;
    runtime.CleanupConfig(state);
    state.words[41u] = 0u;
    state.words[42u] = 0u;
    RestoreBase(state);
}

std::uintptr_t DynamicStripObject::DeletingDestroy(
    DynamicStripObjectState& state, const std::uint32_t flags,
    IDynamicStripObjectRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(state);
    return reinterpret_cast<std::uintptr_t>(&state);
}

}  // namespace pal4
