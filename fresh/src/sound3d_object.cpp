#include "pal4/sound3d_object.h"

#include <array>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 5> kBaseFacets{
    0x84a1d0u, 0x84a1c8u, 0x84a9d8u, 0x84a1bcu, 0x84a178u};
constexpr std::array<std::uint32_t, 5> kDerivedFacets{
    0x84a970u, 0x84a968u, 0x84a95cu, 0x84a950u, 0x84a944u};

void SetFacets(Sound3DObjectState& state,
               const std::array<std::uint32_t, 5>& facets) {
    for (std::uint32_t i = 0u; i < facets.size(); ++i)
        state.words[5u + i] = facets[i];
}
void RestoreBase(Sound3DObjectState& state) {
    for (std::uint32_t i = 5u; i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}
void ChangeInstanceCount(ISound3DObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}
void StoreByte(Sound3DObjectState& state,
               const std::uint32_t offset, const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

}  // namespace

Sound3DObjectState& Sound3DObject::Construct(
    Sound3DObjectState& state, ISound3DObjectRuntime& runtime) {
    state.words[0u] = 0x84b240u;
    for (std::uint32_t i = 1u; i < 5u; ++i)
        state.words[i] = 0u;
    SetFacets(state, kBaseFacets);
    state.words[15u] = 0u;
    StoreByte(state, 64u, runtime.ConstructorStackByte());
    state.words[17u] = 0u;
    state.words[18u] = 0u;
    state.words[19u] = 0u;
    state.words[0u] = 0x84a980u;
    SetFacets(state, kDerivedFacets);
    state.words[13u] = 0u;
    StoreByte(state, 57u, 0u);
    try {
        ChangeInstanceCount(runtime, 1u);
    } catch (...) {
        RestoreBase(state);
        throw;
    }
    return state;
}

void Sound3DObject::Destroy(
    Sound3DObjectState& state, ISound3DObjectRuntime& runtime) {
    state.words[0u] = 0x84a980u;
    SetFacets(state, kDerivedFacets);
    ChangeInstanceCount(runtime, 0xffffffffu);
    runtime.ReleaseSoundPath(state);
    RestoreBase(state);
}

void ISound3DObjectRuntime::ReleaseSoundPath(Sound3DObjectState& state) {
    const auto text = state.words[17u];
    if (text != 0u) {
        const auto ref_address = text - 1u;
        const auto refs = ReadByte(ref_address);
        if (refs == 0u || refs == 0xffu)
            FreeText(ref_address);
        else
            WriteByte(ref_address, static_cast<std::uint8_t>(refs - 1u));
    }
    state.words[17u] = 0u;
    state.words[18u] = 0u;
    state.words[19u] = 0u;
}

std::uintptr_t Sound3DObject::DeletingDestroy(
    Sound3DObjectState& state, const std::uint32_t flags,
    ISound3DObjectRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(state);
    return reinterpret_cast<std::uintptr_t>(&state);
}

std::string_view Sound3DObject::ClassName() {
    return "Sound3DObject";
}

std::uint32_t Sound3DObject::ResourceProperty(
    const Sound3DObjectState& state) {
    return state.words[13u];
}

std::uint8_t Sound3DObject::SetEnabled(
    Sound3DObjectState& state, const std::uint8_t enabled,
    ISound3DObjectRuntime& runtime) {
    const auto active = static_cast<std::uint8_t>(state.words[1u] & 0xffu);
    if (active == 0u)
        return active;
    const auto previous = static_cast<std::uint8_t>((state.words[2u] >> 24u) & 0xffu);
    if (previous != enabled) {
        StoreByte(state, 11u, enabled);
        if (enabled != 0u)
            return runtime.StartPlayback(state, 28u);
    }
    return enabled;
}

}  // namespace pal4
