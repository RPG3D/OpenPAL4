#include "pal4/dynamic_strip_object_facets.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const DynamicStripObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> ((offset % 4u) * 8u));
}

void WriteByte(DynamicStripObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = (offset % 4u) * 8u;
    const auto mask = 0xffu << shift;
    auto& word = state.words[offset / 4u];
    word = (word & ~mask) | (static_cast<std::uint32_t>(value) << shift);
}

std::uint8_t GetVector(const DynamicStripObjectState& state,
                       const std::uint32_t first_offset,
                       std::array<std::uint32_t, 3>& values) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;
    for (std::uint32_t i = 0; i != 3u; ++i)
        values[i] = state.words[first_offset / 4u + i];
    return static_cast<std::uint8_t>(values[2]);
}

std::uint8_t SetVector(DynamicStripObjectState& state,
                       const std::uint32_t first_offset,
                       const std::array<float, 3>& values,
                       const std::uint8_t relative,
                       IDynamicStripTransformRuntime& runtime) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;
    for (std::uint32_t i = 0; i != 3u; ++i) {
        const auto index = first_offset / 4u + i;
        state.words[index] = relative == 0u
            ? std::bit_cast<std::uint32_t>(values[i])
            : std::bit_cast<std::uint32_t>(
                  std::bit_cast<float>(state.words[index]) + values[i]);
    }
    return runtime.UpdateFrame(state);
}

}  // namespace

void DynamicStripVisibilityRuntimeStub::Deactivate(
    DynamicStripObjectState& state) {
    (void)DynamicStripVisibilityFacet::Deactivate(state);
}

std::uint8_t DynamicStripVisibilityFacet::Activate(
    DynamicStripObjectState& state,
    IDynamicStripVisibilityRuntime& runtime) {
    runtime.Deactivate(state);
    if (ReadByte(state, 4u) == 0u)
        return 0u;
    WriteByte(state, 6u, 1u);
    WriteByte(state, 7u, 1u);
    return 1u;
}

std::uint8_t DynamicStripVisibilityFacet::Deactivate(
    DynamicStripObjectState& state) {
    if (ReadByte(state, 4u) != 0u) {
        WriteByte(state, 6u, 0u);
        WriteByte(state, 7u, 0u);
    }
    return 0u;
}

std::uint8_t DynamicStripColorFacet::Get(
    const DynamicStripObjectState& state,
    std::array<std::uint8_t, 4>& rgba) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;
    for (std::uint32_t i = 0; i != 4u; ++i)
        rgba[i] = ReadByte(state, 104u + i);
    return rgba[3];
}

std::uint8_t DynamicStripColorFacet::Set(
    DynamicStripObjectState& state,
    const std::array<std::uint8_t, 4>& rgba) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;
    state.words[26u] = static_cast<std::uint32_t>(rgba[0]) |
                       (static_cast<std::uint32_t>(rgba[1]) << 8u) |
                       (static_cast<std::uint32_t>(rgba[2]) << 16u) |
                       (static_cast<std::uint32_t>(rgba[3]) << 24u);
    return rgba[0];
}

std::uint8_t DynamicStripTransformFacet::GetRotation(
    const DynamicStripObjectState& state,
    std::array<std::uint32_t, 3>& values) {
    return GetVector(state, 80u, values);
}

std::uint8_t DynamicStripTransformFacet::GetScale(
    const DynamicStripObjectState& state,
    std::array<std::uint32_t, 3>& values) {
    return GetVector(state, 68u, values);
}

std::uint8_t DynamicStripTransformFacet::GetPosition(
    const DynamicStripObjectState& state,
    std::array<std::uint32_t, 3>& values) {
    return GetVector(state, 92u, values);
}

std::uint8_t DynamicStripTransformFacet::SetScale(
    DynamicStripObjectState& state,
    const std::array<float, 3>& values, const std::uint8_t relative,
    IDynamicStripTransformRuntime& runtime) {
    return SetVector(state, 68u, values, relative, runtime);
}

std::uint8_t DynamicStripTransformFacet::SetRotation(
    DynamicStripObjectState& state,
    const std::array<float, 3>& values, const std::uint8_t relative,
    IDynamicStripTransformRuntime& runtime) {
    return SetVector(state, 80u, values, relative, runtime);
}

std::uint8_t DynamicStripTransformFacet::SetPosition(
    DynamicStripObjectState& state,
    const std::array<float, 3>& values, const std::uint8_t relative,
    IDynamicStripTransformRuntime& runtime) {
    return SetVector(state, 92u, values, relative, runtime);
}

std::uint8_t DynamicStripTransformFacet::SetTrackPosition(
    DynamicStripObjectState& state,
    const std::array<std::uint32_t, 3>& values,
    IDynamicStripTransformRuntime& runtime) {
    const auto active = ReadByte(state, 4u);
    if (active == 0u)
        return active;
    for (std::uint32_t i = 0; i != 3u; ++i)
        state.words[23u + i] = values[i];
    return runtime.UpdateFrame(state);
}

}  // namespace pal4
