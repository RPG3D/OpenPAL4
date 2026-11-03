#include "pal4/thunder_object.h"

#include <bit>
#include <cstddef>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 9u> kBaseFacets{
    0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u, 0x84a1bcu,
    0x84a1dcu, 0x84a1acu, 0x84abd0u, 0x84a178u};
constexpr std::array<std::uint32_t, 9u> kDerivedFacets{
    0x84ab6cu, 0x84ab60u, 0x84ab54u, 0x84ab4cu, 0x84ab40u,
    0x84ab34u, 0x84ab24u, 0x84ab18u, 0x84ab0cu};

void SetFacets(ThunderObjectState& state,
               const std::array<std::uint32_t, 9u>& facets) {
    for (std::uint32_t i = 0u; i < facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(ThunderObjectState& state) {
    for (std::size_t i = kBaseFacets.size(); i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void StoreByte(ThunderObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    auto& word = state.words[offset / 4u];
    word = (word & ~(0xffu << shift)) |
           (static_cast<std::uint32_t>(value) << shift);
}

void UpdateInstanceCount(IThunderObjectRuntime& runtime,
                         const std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

}  // namespace

ThunderObjectState& ThunderObject::Construct(
    ThunderObjectState& state, IThunderObjectRuntime& runtime) {
    state.words[0u] = 0x84b240u;
    for (std::uint32_t i = 1u; i < 5u; ++i)
        state.words[i] = 0u;
    SetFacets(state, kBaseFacets);

    const auto stack_byte = runtime.ConstructorStackByte();
    StoreByte(state, 60u, 0u);
    StoreByte(state, 61u, 1u);
    StoreByte(state, 62u, 0u);
    StoreByte(state, 63u, 1u);
    for (std::uint32_t offset = 64u; offset <= 66u; ++offset)
        StoreByte(state, offset, 0u);
    state.words[17u] = 4u;
    for (std::uint32_t i = 18u; i <= 26u; ++i)
        state.words[i] = i <= 20u ? 0x3f800000u : 0u;
    state.words[27u] = 0xffffffffu;
    state.words[28u] = 0u;
    state.words[29u] = 0u;
    state.words[34u] = 0u;
    state.words[35u] = 0u;
    state.words[36u] = 0u;
    state.words[37u] = 0u;
    state.words[38u] = 0u;
    state.words[39u] = 0u;
    state.words[40u] = 0x3e4ccccdu;
    state.words[41u] = 0x3f000000u;
    state.words[42u] = 0x3f000000u;
    state.words[43u] = 0x3f800000u;
    state.words[44u] = 0x3dcccccdU;
    state.words[45u] = 0x3ecccccdU;
    StoreByte(state, 184u, stack_byte);
    for (std::uint32_t i = 47u; i <= 49u; ++i)
        state.words[i] = 0u;
    StoreByte(state, 200u, stack_byte);
    for (std::uint32_t i = 51u; i <= 53u; ++i)
        state.words[i] = 0u;
    StoreByte(state, 216u, stack_byte);
    for (std::uint32_t i = 55u; i <= 57u; ++i)
        state.words[i] = 0u;

    state.words[0u] = 0x84ab78u;
    SetFacets(state, kDerivedFacets);
    try {
        UpdateInstanceCount(runtime, 1u);
    } catch (...) {
        RestoreBase(state);
        throw;
    }
    return state;
}

void ThunderObject::Destroy(
    ThunderObjectState& state, IThunderObjectRuntime& runtime) {
    state.words[0u] = 0x84ab78u;
    SetFacets(state, kDerivedFacets);
    UpdateInstanceCount(runtime, 0xffffffffu);
    runtime.ResetErrorField(state, 216u);
    runtime.ResetProperties(state, 60u);
    runtime.CleanupMemoryBlock(state, 200u);
    runtime.CleanupMemoryBlock(state, 184u);
    RestoreBase(state);
}

std::uint32_t ThunderObject::ResourceHandle(
    const ThunderObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u ? state.words[28u] : 0u;
}

std::string_view ThunderObject::ClassName() {
    return "ThunderObject";
}

void ThunderObjectPlayback::SetEnabled(
    ThunderObjectState& state, const std::uint8_t enabled,
    IThunderObjectPlaybackRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return;
    const auto previous = static_cast<std::uint8_t>(state.words[2u] >> 24u);
    if (previous == enabled)
        return;
    state.words[2u] = (state.words[2u] & 0x00ffffffu) |
                      (static_cast<std::uint32_t>(enabled) << 24u);
    if (enabled == 0u)
        return;
    std::array<std::uint32_t, 4u> outputs{};
    runtime.StartFacet(state, outputs);
    const auto duration = runtime.Duration(state);
    state.words[59u] = std::bit_cast<std::uint32_t>(
        static_cast<float>(static_cast<double>(outputs[3u]) / duration));
}

}  // namespace pal4
