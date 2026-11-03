#include "pal4/clump_object.h"

#include <bit>

namespace pal4 {
namespace {

constexpr std::array<std::uint32_t, 12> kBaseFacets{
    0x84a1a0u, 0x84a194u, 0x84a1d0u, 0x84a1c8u,
    0x84a1acu, 0x84a1dcu, 0x84a430u, 0x84a420u,
    0x84a1bcu, 0x84a408u, 0x84a184u, 0x84a178u};
constexpr std::array<std::uint32_t, 12> kDerivedFacets{
    0x84a3a4u, 0x84a398u, 0x84a38cu, 0x84a384u,
    0x84a374u, 0x84a368u, 0x84a358u, 0x84a348u,
    0x84a33cu, 0x84a324u, 0x84a314u, 0x84a308u};

void SetFacets(ClumpObjectState& state,
               const std::array<std::uint32_t, 12>& facets) {
    for (std::size_t i = 0; i < facets.size(); ++i)
        state.words[5u + i] = facets[i];
}

void RestoreBase(ClumpObjectState& state) {
    for (std::size_t i = kBaseFacets.size(); i-- > 0u;)
        state.words[5u + i] = kBaseFacets[i];
    state.words[0u] = 0x84b240u;
}

void WriteByte(ClumpObjectState& state, std::uint32_t offset,
               std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

std::uint8_t ReadByte(const ClumpObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> ((offset % 4u) * 8u));
}

void UpdateInstanceCount(IClumpObjectRuntime& runtime,
                         std::uint32_t delta) {
    const auto address = runtime.SystemInstance() + 164u;
    runtime.WriteWord(address, runtime.ReadWord(address) + delta);
}

std::uint8_t ReadManagerVector(const ClumpObjectState& state,
                               std::array<std::uint32_t, 3>& output,
                               IClumpObjectAccessRuntime& runtime,
                               const std::uint32_t manager_offset) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;
    const auto address = state.words[18u] + manager_offset;
    for (std::uint32_t i = 0u; i < output.size(); ++i)
        output[i] = runtime.ReadWord(address + 4u * i);
    return static_cast<std::uint8_t>(output[2u]);
}

}  // namespace

ClumpObjectState& ClumpObject::Construct(
    ClumpObjectState& state, const std::uint8_t initial_byte,
    IClumpObjectRuntime& runtime) {
    for (std::size_t i = 1u; i <= 4u; ++i)
        state.words[i] = 0u;
    state.words[0u] = 0x84b240u;
    SetFacets(state, kBaseFacets);
    state.words[18u] = 0u;
    state.words[19u] = 0u;

    bool xml_ready = false;
    try {
        runtime.InitializeXml(state, 108u);
        xml_ready = true;
        WriteByte(state, 168u, 0u);
        for (const auto offset : {220u, 236u, 252u, 308u})
            WriteByte(state, offset, initial_byte);
        for (const auto offset : {224u, 228u, 232u, 240u, 244u, 248u,
                                  256u, 260u, 264u, 312u, 316u, 320u})
            state.words[offset / 4u] = 0u;

        state.words[0u] = 0x84a3b0u;
        SetFacets(state, kDerivedFacets);
        for (std::size_t i = 44u; i <= 51u; ++i)
            state.words[i] = 0u;
        UpdateInstanceCount(runtime, 1u);
    } catch (...) {
        if (xml_ready) {
            runtime.ReleaseBuffer(state.words[78u]);
            runtime.ReleaseText(state, 252u);
            runtime.ReleaseText(state, 236u);
            runtime.ReleaseText(state, 220u);
            runtime.DestroyXml(state, 108u);
        }
        RestoreBase(state);
        throw;
    }
    return state;
}

void ClumpObject::Destroy(ClumpObjectState& state,
                          IClumpObjectRuntime& runtime) {
    state.words[0u] = 0x84a3b0u;
    SetFacets(state, kDerivedFacets);
    UpdateInstanceCount(runtime, 0xffffffffu);
    runtime.ReleaseBuffer(state.words[78u]);
    state.words[78u] = 0u;
    state.words[79u] = 0u;
    state.words[80u] = 0u;
    runtime.SetErrorFlag(state, 252u, 1u);
    runtime.SetErrorFlag(state, 236u, 1u);
    runtime.SetErrorFlag(state, 220u, 1u);
    runtime.DestroyXml(state, 108u);
    RestoreBase(state);
}

bool ClumpObject::IsActive(const ClumpObjectState& state,
                           IClumpObjectAccessRuntime& runtime) {
    return ReadByte(state, 4u) != 0u &&
           static_cast<std::int32_t>(
               runtime.ReadWord(state.words[18u] + 40u)) > 0;
}

std::uint32_t ClumpObject::ObjectHandle(
    const ClumpObjectState& state,
    IClumpObjectAccessRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u)
        return 0u;
    const auto object = runtime.ReadWord(state.words[18u] + 20u);
    return object == 0u ? 0u : runtime.ReadWord(object + 4u);
}

std::uint32_t ClumpObject::Object(
    const ClumpObjectState& state,
    IClumpObjectAccessRuntime& runtime) {
    return ReadByte(state, 4u) == 0u ? 0u :
           runtime.ReadWord(state.words[18u] + 20u);
}

std::uint8_t ClumpObject::GetPosition(
    const ClumpObjectState& state, std::array<std::uint32_t, 3>& output,
    IClumpObjectAccessRuntime& runtime) {
    return ReadManagerVector(state, output, runtime, 172u);
}

std::uint8_t ClumpObject::GetRotation(
    const ClumpObjectState& state, std::array<std::uint32_t, 3>& output,
    IClumpObjectAccessRuntime& runtime) {
    return ReadManagerVector(state, output, runtime, 160u);
}

std::uint8_t ClumpObject::GetScale(
    const ClumpObjectState& state, std::array<std::uint32_t, 3>& output,
    IClumpObjectAccessRuntime& runtime) {
    return ReadManagerVector(state, output, runtime, 184u);
}

std::string_view ClumpObject::ClassName() {
    return "ClumpObject";
}

void ClumpObject::SetTimedMode(ClumpObjectState& state,
                               const std::uint32_t enabled,
                               IClumpObjectPlaybackRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u)
        return;
    const auto new_mode = static_cast<std::uint8_t>(enabled);
    if (ReadByte(state, 11u) == new_mode)
        return;
    WriteByte(state, 11u, new_mode);
    if (new_mode == 0u)
        return;

    std::array<std::int32_t, 4> values{};
    runtime.QueryTaskValues(state, 36u, values);
    const auto numerator = static_cast<float>(values[3u]);
    const auto ratio = static_cast<float>(numerator / runtime.Duration(state));
    state.words[43u] = std::bit_cast<std::uint32_t>(ratio);
}

}  // namespace pal4
