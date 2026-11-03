#include "pal4/effect_camera_properties.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t ReadByte(const EffectCameraObjectState& state,
                      const std::uint32_t offset) {
    return static_cast<std::uint8_t>(
        state.words[offset / 4u] >> (8u * (offset % 4u)));
}

void WriteByte(EffectCameraObjectState& state, const std::uint32_t offset,
               const std::uint8_t value) {
    const auto shift = 8u * (offset % 4u);
    const auto mask = 0xffu << shift;
    state.words[offset / 4u] =
        (state.words[offset / 4u] & ~mask) |
        (static_cast<std::uint32_t>(value) << shift);
}

void CopyWords(EffectCameraObjectState& state,
               const std::uint32_t output_index,
               const std::array<std::uint32_t, 3>& values) {
    for (std::uint32_t index = 0u; index < values.size(); ++index)
        state.words[output_index + index] = values[index];
}

void AddFloats(EffectCameraObjectState& state,
               const std::uint32_t output_index,
               const std::array<std::uint32_t, 3>& values) {
    for (std::uint32_t index = 0u; index < values.size(); ++index) {
        const float current =
            std::bit_cast<float>(state.words[output_index + index]);
        const float delta = std::bit_cast<float>(values[index]);
        state.words[output_index + index] =
            std::bit_cast<std::uint32_t>(current + delta);
    }
}

std::uint8_t GetFields(const EffectCameraObjectState& state,
                       const std::uint32_t word_index,
                       std::array<std::uint32_t, 3>& output,
                       const std::uint8_t third_output_address_low_byte) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 80u) == 0u)
        return 0u;
    if (ReadByte(state, 44u) != 0u) {
        output = {};
        return third_output_address_low_byte;
    }
    for (std::uint32_t index = 0u; index < output.size(); ++index)
        output[index] = state.words[word_index + index];
    return static_cast<std::uint8_t>(output[2u]);
}

}  // namespace

std::uint8_t EffectCameraProperties::GetPosition(
    const EffectCameraObjectState& state,
    std::array<std::uint32_t, 3>& output,
    const std::uint8_t third_output_address_low_byte) {
    return GetFields(state, 25u, output, third_output_address_low_byte);
}

std::uint8_t EffectCameraProperties::GetRotation(
    const EffectCameraObjectState& state,
    std::array<std::uint32_t, 3>& output,
    const std::uint8_t third_output_address_low_byte) {
    return GetFields(state, 22u, output, third_output_address_low_byte);
}

std::uint8_t EffectCameraProperties::SetPosition(
    EffectCameraObjectState& state,
    const std::array<std::uint32_t, 3>& values,
    const std::uint8_t relative) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 80u) == 0u)
        return 0u;
    if (ReadByte(state, 44u) != 0u) {
        CopyWords(state, 25u, values);
        return static_cast<std::uint8_t>(values[1u]);
    }
    if (relative != 0u) {
        AddFloats(state, 25u, values);
        return relative;
    }
    CopyWords(state, 25u, values);
    return static_cast<std::uint8_t>(values[2u]);
}

std::uint8_t EffectCameraProperties::SetRotation(
    EffectCameraObjectState& state,
    const std::array<std::uint32_t, 3>& values,
    const std::uint8_t relative,
    IEffectCameraPropertiesRuntime& runtime) {
    if (ReadByte(state, 4u) == 0u || ReadByte(state, 80u) == 0u)
        return 0u;
    const auto inherit = ReadByte(state, 44u);
    if (inherit != 0u)
        return inherit;
    if (ReadByte(state, 47u) != 0u) {
        WriteByte(state, 47u, 0u);
        for (std::uint32_t index = 0u; index < 3u; ++index)
            state.words[28u + index] = state.words[22u + index];
        runtime.CalculateAngles(state.words[31u] + 16u, state, 88u);
    }
    if (relative != 0u) {
        AddFloats(state, 22u, values);
        return relative;
    }
    CopyWords(state, 22u, values);
    return static_cast<std::uint8_t>(values[2u]);
}

}  // namespace pal4
