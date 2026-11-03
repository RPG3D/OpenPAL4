#include "pal4/sequence_anim_properties.h"

#include <bit>

namespace pal4 {
namespace {

bool Active(const SequenceAnimObjectState& state) {
    return (state.words[1u] & 0xffu) != 0u;
}

}  // namespace

std::uint8_t SequenceAnimProperties::GetRotation(
    const SequenceAnimObjectState& state,
    std::array<std::uint32_t, 3>& output,
    const std::uint32_t third_output_address,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return 0u;
    output[0u] = runtime.ReadWord(state.words[30u] + 24u);
    output[1u] = 0u;
    output[2u] = 0u;
    return static_cast<std::uint8_t>(third_output_address);
}

std::uint8_t SequenceAnimProperties::GetScale(
    const SequenceAnimObjectState& state,
    std::array<std::uint32_t, 3>& output,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return 0u;
    output[0u] = runtime.ReadWord(state.words[30u] + 40u);
    output[1u] = runtime.ReadWord(state.words[30u] + 44u);
    output[2u] = 0u;
    return static_cast<std::uint8_t>(output[1u]);
}

std::uint8_t SequenceAnimProperties::GetPosition(
    const SequenceAnimObjectState& state,
    std::array<std::uint32_t, 3>& output,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return 0u;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        output[i] = runtime.ReadWord(state.words[30u] + 28u + i * 4u);
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint32_t SequenceAnimProperties::SetRotation(
    const SequenceAnimObjectState& state, const float rotation,
    const std::uint8_t relative, const std::uint32_t entry_eax,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return entry_eax & 0xffffff00u;
    const auto resource = state.words[30u];
    if (relative != 0u) {
        const auto current = std::bit_cast<float>(runtime.ReadWord(resource + 24u));
        runtime.WriteWord(resource + 24u,
                          std::bit_cast<std::uint32_t>(current + rotation));
        return resource;
    }
    runtime.WriteWord(resource + 24u, std::bit_cast<std::uint32_t>(rotation));
    return entry_eax & 0xffffff00u;
}

std::uint32_t SequenceAnimProperties::SetScale(
    const SequenceAnimObjectState& state, const float width,
    const float height, const std::uint8_t relative,
    const std::uint32_t entry_eax,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return entry_eax & 0xffffff00u;
    const auto resource = state.words[30u];
    float next_width = width;
    float next_height = height;
    if (relative != 0u) {
        next_width += std::bit_cast<float>(runtime.ReadWord(resource + 40u));
        next_height += std::bit_cast<float>(runtime.ReadWord(resource + 44u));
    }
    runtime.WriteWord(resource + 40u, std::bit_cast<std::uint32_t>(next_width));
    runtime.WriteWord(resource + 44u, std::bit_cast<std::uint32_t>(next_height));
    return std::bit_cast<std::uint32_t>(next_height);
}

std::uint32_t SequenceAnimProperties::SetPosition(
    const SequenceAnimObjectState& state,
    const std::array<float, 3>& position,
    const std::uint8_t relative, const std::uint32_t entry_eax,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return entry_eax & 0xffffff00u;
    const auto resource = state.words[30u];
    std::array<std::uint32_t, 3> next{};
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        auto value = position[i];
        if (relative != 0u) {
            const auto current = std::bit_cast<float>(
                runtime.ReadWord(resource + 28u + i * 4u));
            value = current + value;
        }
        next[i] = std::bit_cast<std::uint32_t>(value);
    }
    for (std::uint32_t i = 0u; i < 3u; ++i)
        runtime.WriteWord(resource + 28u + i * 4u, next[i]);
    return runtime.InitializeTransform(resource);
}

std::uint8_t SequenceAnimProperties::SetPositionWords(
    const SequenceAnimObjectState& state,
    const std::array<std::uint32_t, 3>& position,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto resource = state.words[30u];
    for (std::uint32_t i = 0u; i < 3u; ++i)
        runtime.WriteWord(resource + 28u + i * 4u, position[i]);
    return static_cast<std::uint8_t>(runtime.InitializeTransform(resource));
}

std::uint8_t SequenceAnimProperties::GetColor(
    const SequenceAnimObjectState& state,
    std::array<std::uint8_t, 4>& output,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return 0u;
    const auto geometry = runtime.ReadWord(state.words[30u] + 48u);
    const auto rgba = runtime.ReadWord(geometry + 28u);
    for (std::uint32_t i = 0u; i < 4u; ++i)
        output[i] = static_cast<std::uint8_t>(rgba >> (i * 8u));
    return output[3u];
}

std::uint8_t SequenceAnimProperties::SetColor(
    const SequenceAnimObjectState& state,
    const std::array<std::uint8_t, 4>& channels,
    ISequenceAnimPropertiesRuntime& runtime) {
    if (!Active(state))
        return 0u;
    std::uint32_t rgba = 0u;
    for (std::uint32_t i = 0u; i < 4u; ++i)
        rgba |= static_cast<std::uint32_t>(channels[i]) << (i * 8u);
    const auto geometry = runtime.ReadWord(state.words[30u] + 48u);
    return runtime.SetGeometryColor(geometry, rgba);
}

}  // namespace pal4
