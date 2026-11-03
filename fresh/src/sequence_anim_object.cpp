#include "pal4/sequence_anim_object.h"

#include <cstddef>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 8> kBaseFacets{
    0x84a1dcu, 0x84a1d0u, 0x84a1c8u, 0x84a1bcu,
    0x84a1acu, 0x84a1a0u, 0x84a194u, 0x84a178u};
constexpr std::array<std::uint32_t, 8> kDerivedFacets{
    0x84a8b8u, 0x84a8acu, 0x84a8a4u, 0x84a898u,
    0x84a888u, 0x84a87cu, 0x84a870u, 0x84a864u};

void SetFacets(SequenceAnimObjectState& state,
               const std::array<std::uint32_t, 8>& facets) {
    for (std::size_t i = 0u; i != facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(SequenceAnimObjectState& state) {
    for (std::size_t i = kBaseFacets.size(); i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void ChangeInstanceCount(ISequenceAnimObjectRuntime& runtime,
                         std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

SequenceAnimObjectState& SequenceAnimObject::Construct(
    SequenceAnimObjectState& state,
    ISequenceAnimObjectRuntime& runtime) {
    state.words[0u] = 0x84b240u;
    for (std::size_t i = 1u; i != 5u; ++i)
        state.words[i] = 0u;
    SetFacets(state, kBaseFacets);
    state.words[13u] = 0u;
    state.words[30u] = 0u;
    state.words[0u] = 0x84a8c8u;
    SetFacets(state, kDerivedFacets);
    try {
        ChangeInstanceCount(runtime, 1u);
    } catch (...) {
        RestoreBase(state);
        throw;
    }
    return state;
}

void SequenceAnimObject::Destroy(
    SequenceAnimObjectState& state, ISequenceAnimObjectRuntime& runtime) {
    state.words[0u] = 0x84a8c8u;
    SetFacets(state, kDerivedFacets);
    ChangeInstanceCount(runtime, 0xffffffffu);
    RestoreBase(state);
}

std::uint32_t SequenceAnimObject::ResourceProperty(
    const SequenceAnimObjectState& state,
    ISequenceAnimObjectRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.ReadWord(state.words[30u] + 20u);
}

std::string_view SequenceAnimObject::ClassName() {
    return "SequnceAnimObject";
}

}  // namespace pal4
