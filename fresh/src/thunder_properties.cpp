#include "pal4/thunder_properties.h"

#include <bit>

namespace pal4 {
namespace {

std::uint8_t Get(const ThunderObjectState& state, const std::uint32_t offset,
                 std::array<std::uint32_t, 3u>& output) {
    const auto active = static_cast<std::uint8_t>(state.words[1u]);
    if (active == 0u)
        return active;
    for (std::uint32_t i = 0u; i < 3u; ++i)
        output[i] = state.words[offset / 4u + i];
    return static_cast<std::uint8_t>(output[2u]);
}

std::uint32_t Set(ThunderObjectState& state, const std::uint32_t offset,
                  const std::array<float, 3u>& values, const bool relative,
                  IThunderPropertyRuntime& runtime) {
    if ((state.words[1u] & 0xffu) == 0u)
        return 0u;
    for (std::uint32_t i = 0u; i < 3u; ++i) {
        const auto current = std::bit_cast<float>(state.words[offset / 4u + i]);
        const auto value = relative ? current + values[i] : values[i];
        state.words[offset / 4u + i] = std::bit_cast<std::uint32_t>(value);
    }
    return runtime.UpdateFrame(state);
}

}  // namespace

std::uint8_t ThunderProperties::GetRotation(
    const ThunderObjectState& state,
    std::array<std::uint32_t, 3u>& output) {
    return Get(state, 84u, output);
}

std::uint8_t ThunderProperties::GetScale(
    const ThunderObjectState& state,
    std::array<std::uint32_t, 3u>& output) {
    return Get(state, 72u, output);
}

std::uint8_t ThunderProperties::GetPosition(
    const ThunderObjectState& state,
    std::array<std::uint32_t, 3u>& output) {
    return Get(state, 96u, output);
}

std::uint32_t ThunderProperties::SetScale(
    ThunderObjectState& state, const std::array<float, 3u>& values,
    const bool relative, IThunderPropertyRuntime& runtime) {
    return Set(state, 72u, values, relative, runtime);
}

std::uint32_t ThunderProperties::SetRotation(
    ThunderObjectState& state, const std::array<float, 3u>& values,
    const bool relative, IThunderPropertyRuntime& runtime) {
    return Set(state, 84u, values, relative, runtime);
}

std::uint32_t ThunderProperties::SetPosition(
    ThunderObjectState& state, const std::array<float, 3u>& values,
    const bool relative, IThunderPropertyRuntime& runtime) {
    return Set(state, 96u, values, relative, runtime);
}

}  // namespace pal4
