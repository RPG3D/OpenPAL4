#include "pal4/cylinder_object.h"

#include <cstddef>
#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 10> kBaseFacets{
    0x84a1dcu, 0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1bcu, 0x84a1acu, 0x84a184u, 0x84a408u, 0x84a178u};
constexpr std::array<std::uint32_t, 10> kDerivedFacets{
    0x84a4e0u, 0x84a4d4u, 0x84a4c8u, 0x84a4bcu, 0x84a4b4u,
    0x84a4a8u, 0x84a498u, 0x84a488u, 0x84a470u, 0x84a464u};

void SetFacets(CylinderObjectState& state,
               const std::array<std::uint32_t, 10>& facets) {
    for (std::size_t i = 0u; i < facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(CylinderObjectState& state) {
    for (std::size_t i = kBaseFacets.size(); i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void UpdateInstanceCount(ICylinderObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

CylinderObjectState& CylinderObject::Construct(
    CylinderObjectState& state, const std::uint8_t initial_byte,
    ICylinderObjectRuntime& runtime) {
    for (std::size_t i = 1u; i <= 4u; ++i)
        state.words[i] = 0u;
    state.words[0u] = 0x84b240u;
    SetFacets(state, kBaseFacets);
    state.words[22u] = 0u;
    state.words[23u] = 0u;
    state.words[35u] = (state.words[35u] & 0xffffff00u) | initial_byte;
    state.words[36u] = 0u;
    state.words[37u] = 0u;
    state.words[38u] = 0u;
    state.words[0u] = 0x84a4f0u;
    SetFacets(state, kDerivedFacets);
    try {
        UpdateInstanceCount(runtime, 1u);
    } catch (...) {
        runtime.FreeBuffer(state.words[36u]);
        RestoreBase(state);
        throw;
    }
    return state;
}

std::uint32_t CylinderObject::ForwardAction(
    CylinderObjectState& state, const std::array<std::uint32_t, 3>& args,
    ICylinderObjectRuntime& runtime) {
    return runtime.ForwardAction(state, args, 0u);
}

void CylinderObject::Destroy(CylinderObjectState& state,
                             ICylinderObjectRuntime& runtime) {
    state.words[0u] = 0x84a4f0u;
    SetFacets(state, kDerivedFacets);
    UpdateInstanceCount(runtime, 0xffffffffu);
    runtime.FreeBuffer(state.words[36u]);
    state.words[36u] = 0u;
    state.words[37u] = 0u;
    state.words[38u] = 0u;
    RestoreBase(state);
}

std::uintptr_t CylinderObject::DeletingDestroy(
    CylinderObjectState& state, const std::uint32_t flags,
    ICylinderObjectRuntime& runtime) {
    Destroy(state, runtime);
    if ((flags & 1u) != 0u)
        runtime.FreeObject(state);
    return reinterpret_cast<std::uintptr_t>(&state);
}

std::uint32_t CylinderObject::ResourceHandle(
    const CylinderObjectState& state,
    ICylinderObjectAccessRuntime& runtime) {
    return (state.words[1u] & 0xffu) == 0u ? 0u :
           runtime.ReadWord(state.words[22u] + 8u);
}

std::uint32_t CylinderObject::Resource(
    const CylinderObjectState& state,
    ICylinderObjectAccessRuntime& runtime) {
    return (state.words[1u] & 0xffu) == 0u ? 0u :
           runtime.ReadWord(state.words[22u] + 4u);
}

std::string_view CylinderObject::ClassName() {
    return "CylinderObject";
}

void CylinderObject::SetTimedMode(
    CylinderObjectState& state, const std::uint32_t enabled,
    ICylinderObjectPlaybackRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return;
    const auto new_mode = static_cast<std::uint8_t>(enabled);
    const auto old_mode = static_cast<std::uint8_t>(state.words[2u] >> 24u);
    if (old_mode == new_mode)
        return;
    state.words[2u] = (state.words[2u] & 0x00ffffffu) |
                      (static_cast<std::uint32_t>(new_mode) << 24u);
    if (new_mode == 0u)
        return;
    std::array<std::int32_t, 4> values{};
    runtime.QueryTaskValues(state, 44u, values);
    state.words[24u] = std::bit_cast<std::uint32_t>(
        static_cast<float>(values[3u]) /
        static_cast<float>(runtime.Duration(state)));
}

}  // namespace pal4
