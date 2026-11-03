#include "pal4/rtk_prt_std_atomic_object.h"

#include <cstddef>
#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 10u> kBaseFacets{
    0x84a1dcu, 0x84a1d0u, 0x84a1a0u, 0x84a194u, 0x84a1c8u,
    0x84a1bcu, 0x84a1acu, 0x84a830u, 0x84a408u, 0x84a178u};
constexpr std::array<std::uint32_t, 10u> kDerivedFacets{
    0x84a7ccu, 0x84a7c0u, 0x84a7b4u, 0x84a7a8u, 0x84a7a0u,
    0x84a794u, 0x84a784u, 0x84a778u, 0x84a760u, 0x84a754u};

void SetFacets(RtkPrtStdAtomicObjectState& state,
               const std::array<std::uint32_t, 10u>& facets) {
    for (std::size_t i = 0u; i != facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(RtkPrtStdAtomicObjectState& state) {
    for (std::size_t i = kBaseFacets.size(); i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void ChangeInstanceCount(IRtkPrtStdAtomicObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

RtkPrtStdAtomicObjectState& RtkPrtStdAtomicObject::Construct(
    RtkPrtStdAtomicObjectState& state,
    IRtkPrtStdAtomicObjectRuntime& runtime) {
    state.words[0u] = 0x84b240u;
    for (std::size_t i = 1u; i != 5u; ++i)
        state.words[i] = 0u;
    SetFacets(state, kBaseFacets);
    state.words[23u] = 0u;
    state.words[0u] = 0x84a7d8u;
    SetFacets(state, kDerivedFacets);
    try {
        ChangeInstanceCount(runtime, 1u);
    } catch (...) {
        RestoreBase(state);
        throw;
    }
    return state;
}

double RtkPrtStdAtomicObject::GetDuration(
    const RtkPrtStdAtomicObjectState& state,
    IRtkPrtStdAtomicObjectRuntime& runtime) {
    const auto resource = state.words[23u];
    const auto bias = runtime.ReadFloat(resource + 280u);
    const auto life = runtime.ReadFloat(resource + 276u);
    return static_cast<double>(bias) + static_cast<double>(life);
}

void RtkPrtStdAtomicObject::Destroy(
    RtkPrtStdAtomicObjectState& state,
    IRtkPrtStdAtomicObjectRuntime& runtime) {
    state.words[0u] = 0x84a7d8u;
    SetFacets(state, kDerivedFacets);
    ChangeInstanceCount(runtime, 0xffffffffu);
    RestoreBase(state);
}

std::uint8_t RtkPrtStdAtomicObject::Update(
    const RtkPrtStdAtomicObjectState& state,
    IRtkPrtStdAtomicUpdateRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    const auto duration = std::bit_cast<float>(state.words[22u]);
    if (!(duration > 0.0f))
        return 0u;
    const auto elapsed = static_cast<double>(duration) *
                         static_cast<double>(std::bit_cast<float>(
                             0x3b03126fu));
    std::uint8_t result = 0u;
    for (std::uint32_t i = 0u; i != 500u; ++i) {
        const auto argument =
            runtime.ReadWord(state.words[4u] + 48u);
        result = runtime.UpdateState(
            state.words[23u], elapsed, argument, 0u);
    }
    return result;
}

std::uint32_t RtkPrtStdAtomicObject::ResourceProperty(
    const RtkPrtStdAtomicObjectState& state,
    IRtkPrtStdAtomicObjectRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    return runtime.ReadWord(state.words[23u] + 44u);
}

std::string_view RtkPrtStdAtomicObject::ClassName() {
    return "RtkPrtStdAtomicObject";
}

}  // namespace pal4
